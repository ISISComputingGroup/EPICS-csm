#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <epicsStdlib.h>

#include <registryFunction.h>

#include <alarm.h>
#include <dbAccess.h>
#include <dbStaticLib.h>
#include <dbEvent.h>
#include <dbFldTypes.h>
#include <devSup.h>
#include <errMdef.h>
#include <errlog.h>
#include <special.h>
#include <recSup.h>
#include <recGbl.h>
#define GEN_SIZE_OFFSET
#include <cvtRecord.h>
#undef  GEN_SIZE_OFFSET
#include <menuIvoa.h>

#include <csmbase.h>

#include <epicsExport.h>

/*#include "menuCvtMethod.h"*/
/*#include "menuCvtInitState.h"*/

/* Warning: these must correspond exactly to the size property of the
   fields in the dbd file */
#define BDIR_SIZE 40
#define TDIR_SIZE 40
#define SPEC_SIZE 40

/* error message macros */

#if defined(_WIN32) && defined(_MSC_VER)
#define genmsg(sevr,name,msg,...)\
    errlogSevPrintf(sevr,"%s(%s): " msg "\n", __FUNCTION__, name, __VA_ARGS__ )
#define nerrmsg(name,msg,...) genmsg(errlogFatal,name,msg, __VA_ARGS__ )
#define errmsg(msg,...) genmsg(errlogFatal,pcvt->name,msg, __VA_ARGS__ )
#else
#define genmsg(sevr,name,msg,args...)\
    errlogSevPrintf(sevr,"%s(%s): " msg "\n", __FUNCTION__, name, ## args)
#define nerrmsg(name,msg,args...) genmsg(errlogFatal,name,msg, ## args)
#define errmsg(msg,args...) genmsg(errlogFatal,pcvt->name,msg, ## args)
#endif /* _WIN32 && _MSC_VER */

/* standard EPICS record support stuff */

/* Create RSET - Record Support Entry Table*/
#define report NULL
static long initialize();
static long init_record();
static long process();
static long special();

#define get_value NULL
#define cvt_dbaddr NULL
#define get_array_info NULL
#define put_array_info NULL
static long get_units();
static long get_precision();

#define get_enum_str NULL
#define get_enum_strs NULL
#define put_enum_str NULL
static long get_graphic_double();
static long get_control_double();
static long get_alarm_double();

struct rset cvtRSET = {
    RSETNUMBER,
    report,
    initialize,
    init_record,
    process,
    special,
    get_value,
    cvt_dbaddr,
    get_array_info,
    put_array_info,
    get_units,
    get_precision,
    get_enum_str,
    get_enum_strs,
    put_enum_str,
    get_graphic_double,
    get_control_double,
    get_alarm_double
};

epicsExportAddress(rset,cvtRSET);

/*
 * General Remarks
 * ===============
 *
 * This record type supports conversion of one or two floating point values
 * into one resulting floating point value. The field METH specifies the kind
 * of conversion: LINEAR, via a custom SUBROUTINE, or via one- (1D) or
 * two-dimensional (2D) conversion TABLE. More specifically, if METH is
 *
 * menuCvtMethodLinear:
 *     VAL = XSLO * X + YSLO * Y + VOFF
 *
 * menuCvtMethodSubroutine:
 *     SPEC should be the name of a global subroutine. CSUB is set to the
 *     address of the subroutine.
 *     VAL = CSUB(X, Y, &DPVT)
 *
 * menuCvtMethod1DTable:
 * menuCvtMethod1DTableInverted:
 * menuCvtMethod2DTable:
 *     Conversion uses the csm module. SPEC should be the filename
 *     of the table. CSUB is set to the csm_function handle returned by csm.
 *     VAL = csm_y(CSUB, X)         for one-dimensional tables
 *     VAL = csm_x(CSUB, Y)         for one-dimensional inverted tables
 *     VAL = csm_z(CSUB, X, Y)      two-dimensional tables
 */

/*
 * If METH==menuCvtMethodSubroutine, then SPEC should be the name of a
 * conversion subroutine with the following type. First two args are
 * INPX and INPY, the 3rd arg is a pointer to the record's DPVT field.
 * The subroutine may allocate a private structure and store its address
 * in the supplied pointer in order to store data between separate calls.
 */
typedef double cvt_subroutine(double,double,void**);

typedef double (*user1DTableSub_t)(bool, double *, double *, int, double, void **);

/* Values for field DRTY (dirty bits) */
#define DRTY_NONE 0x00
#define DRTY_METH 0x01
#define DRTY_SPEC 0x02
#define DRTY_BDIR 0x04
#define DRTY_TDIR 0x08
#define DRTY_ISTA 0x10
#define DRTY_X    0x20
#define DRTY_Y    0x40

static void checkAlarms(struct cvtRecord *pcvt);
static long readInputs(struct cvtRecord *pcvt);
static long convert(struct cvtRecord *pcvt);
static void monitor(struct cvtRecord *pcvt);
static long checkInit(struct cvtRecord *pcvt);
static long reinitConversion(struct cvtRecord *pcvt);
static long initConversion(const char *name, const char *bdir, const char *tdir,
    menuCvtMethod meth, const char *spec, void **psub, struct cvtRecord *pcvt);

static epicsMessageQueueId initConversionQ;

/*
 * Invariants:
 *
 *  o METH and SPEC specify the currently active conversion.
 *  o Writing any non-zero value to INIT re-initializes
 *    conversion with the values of NMET and NSPE at that moment.
 *  o Each record is enqueued in initConversionQ at most once.
 *  o New conversion values are written back if and only if
 *    initConversion succeeds and state!=Again.
 *    => If conversion re-init either fails (for whatever reason)
 *    or must be repeated (state==Again), values are *not* written back.
 *  o NMET and NSPE are written by record support only once
 *    during record initialization (init_record).
 */

static long init_record(struct cvtRecord *pcvt, int pass)
{
    void *sub;

    if (pass == 0) {
        /* set new conversion parameters equal to to configured ones */
        pcvt->nmet = pcvt->meth;
        strncpy(pcvt->nbdi, pcvt->bdir, BDIR_SIZE);
        strncpy(pcvt->ntdi, pcvt->tdir, TDIR_SIZE);
        strncpy(pcvt->nspe, pcvt->spec, SPEC_SIZE);
        return 0;
    }

    /* initialize input links */
    if (pcvt->inpx.type == CONSTANT) {
        recGblInitConstantLink(&pcvt->inpx, DBF_DOUBLE, &pcvt->x);
    }
    if (pcvt->inpy.type == CONSTANT) {
        recGblInitConstantLink(&pcvt->inpy, DBF_DOUBLE, &pcvt->y);
    }
    if (pcvt->iaml.type == CONSTANT) {
        recGblInitConstantLink(&pcvt->iaml, DBF_UCHAR, &pcvt->iaom);
    }
    if (pcvt->iavl.type == CONSTANT) {
        recGblInitConstantLink(&pcvt->iavl, DBF_DOUBLE, &pcvt->iaov);
    }

    /* try to initialize conversion as specified */
    if (initConversion(pcvt->name, pcvt->nbdi, pcvt->ntdi, pcvt->nmet, pcvt->nspe, &sub, pcvt)) {
        pcvt->ista = menuCvtInitStateError;
        pcvt->drty |= DRTY_ISTA;
        return -1;
    }
    pcvt->csub = sub;
    return 0;
}

static long process(struct cvtRecord *pcvt)
{
    long status = 0;

    pcvt->pact = TRUE;
    status = dbGetLink(&pcvt->inil, DBR_UCHAR, &pcvt->init, 0, 0);
    pcvt->pact = FALSE;

    if (status) {
        recGblSetSevr(pcvt, LINK_ALARM, INVALID_ALARM);
        goto error;
    }
    if (checkInit(pcvt)) {
        recGblSetSevr(pcvt, SOFT_ALARM, INVALID_ALARM);
        recGblResetAlarms(pcvt);
        pcvt->pact = TRUE;
        return -1;
    }

    pcvt->pact = TRUE;
    status = readInputs(pcvt);
    pcvt->pact = FALSE;

    if (status) {
        goto error;
    }

    status = convert(pcvt);

error:
    checkAlarms(pcvt);

    pcvt->pact = TRUE;
    if (pcvt->nsev < INVALID_ALARM) {
        status = dbPutLink(&pcvt->out, DBR_DOUBLE, &pcvt->val, 1);
    }
    else {
        switch (pcvt->ivoa) {
        case (menuIvoaSet_output_to_IVOV):
            pcvt->val = pcvt->ivov;
            /* note: this falls through to the case below */
        case (menuIvoaContinue_normally):
            status = dbPutLink(&pcvt->out, DBR_DOUBLE, &pcvt->val, 1);
        case (menuIvoaDon_t_drive_outputs):
            break;
        default:
            status = S_db_badField;
            errmsg("internal error: Illegal value in IVOA field");
            recGblSetSevr(pcvt, SOFT_ALARM, INVALID_ALARM);
            recGblResetAlarms(pcvt);
            return status;
        }
    }

    recGblGetTimeStamp(pcvt);
    monitor(pcvt);
    recGblFwdLink(pcvt);

    pcvt->pact = FALSE;

    return status;
}

static long checkInit(struct cvtRecord *pcvt)
{
    if (!pcvt->init) {
        return 0;
    }
    pcvt->init = 0;
    switch (pcvt->ista) {
    case menuCvtInitStateDone:
    case menuCvtInitStateError:
        pcvt->ista = menuCvtInitStateInProgress;
        pcvt->drty |= DRTY_ISTA;
        if (reinitConversion(pcvt)) {
            /* this is fatal */
            pcvt->ista = menuCvtInitStateError;
            pcvt->pact = TRUE;
            return -1;
        }
        break;
    case menuCvtInitStateInProgress:
        pcvt->ista = menuCvtInitStateAgain;
        pcvt->drty |= DRTY_ISTA;
        break;
    case menuCvtInitStateAgain:
        break;
    default:
        errmsg("internal error: illegal value %d in field ISTA", pcvt->ista);
        pcvt->pact = TRUE;
        return -1;
    }
    checkAlarms(pcvt);
    monitor(pcvt);
    return 0;
}

static long special(struct dbAddr *paddr, int after)
{
    struct cvtRecord *pcvt = (struct cvtRecord *)(paddr->precord);
    int fieldIndex = dbGetFieldIndex(paddr);

    if (!after) return 0;
    if (paddr->special==SPC_MOD) {
        switch (fieldIndex) {
        case cvtRecordINIT:
            if (checkInit(pcvt)) {
                return -1;
            }
            return 0;
        default:
            errmsg("internal error: special called for wrong field");
            pcvt->pact = TRUE;
            return -1;
        }
    }
    errmsg("internal error: special called with wrong special type");
    pcvt->pact = TRUE;
    return -1;
}

static long get_units(struct dbAddr *paddr, char *units)
{
    struct cvtRecord *pcvt = (struct cvtRecord *)paddr->precord;

    strncpy(units, pcvt->egu, DB_UNITS_SIZE);
    return 0;
}

static long get_precision(struct dbAddr *paddr, long *precision)
{
    struct cvtRecord *pcvt = (struct cvtRecord *)paddr->precord;
    int fieldIndex = dbGetFieldIndex(paddr);

    switch (fieldIndex) {
    case cvtRecordVAL:
        *precision = pcvt->prec;
        break;
    default:
        recGblGetPrec(paddr, precision);
    }
    return 0;
}

static long get_graphic_double(struct dbAddr *paddr, struct dbr_grDouble *pgd)
{
    struct cvtRecord *pcvt = (struct cvtRecord *)paddr->precord;
    int fieldIndex = dbGetFieldIndex(paddr);

    switch (fieldIndex) {
    case cvtRecordVAL:
    case cvtRecordHIHI:
    case cvtRecordHIGH:
    case cvtRecordLOW:
    case cvtRecordLOLO:
        pgd->upper_disp_limit = pcvt->hopr;
        pgd->lower_disp_limit = pcvt->lopr;
        break;
    default:
        recGblGetGraphicDouble(paddr, pgd);
    }
    return 0;
}

static long get_control_double(struct dbAddr *paddr, struct dbr_ctrlDouble *pcd)
{
    struct cvtRecord *pcvt = (struct cvtRecord *)paddr->precord;
    int fieldIndex = dbGetFieldIndex(paddr);

    switch (fieldIndex) {
    case cvtRecordVAL:
    case cvtRecordHIHI:
    case cvtRecordHIGH:
    case cvtRecordLOW:
    case cvtRecordLOLO:
        pcd->upper_ctrl_limit = pcvt->drvh;
        pcd->lower_ctrl_limit = pcvt->drvl;
        break;
    default:
        recGblGetControlDouble(paddr, pcd);
    }
    return 0;
}

static long get_alarm_double(struct dbAddr *paddr, struct dbr_alDouble *pad)
{
    struct cvtRecord *pcvt = (struct cvtRecord *)paddr->precord;
    int fieldIndex = dbGetFieldIndex(paddr);

    if (fieldIndex == cvtRecordVAL) {
        pad->upper_alarm_limit = pcvt->hihi;
        pad->upper_warning_limit = pcvt->high;
        pad->lower_warning_limit = pcvt->low;
        pad->lower_alarm_limit = pcvt->lolo;
    }
    else
        recGblGetAlarmDouble(paddr, pad);
    return 0;
}

static void checkAlarms(struct cvtRecord *pcvt)
{
    double hyst, lalm, val;
    double hihi, high, low, lolo;
    epicsEnum16 hhsv, llsv, hsv, lsv;

    if (pcvt->udf == TRUE) {
        recGblSetSevr(pcvt, UDF_ALARM, INVALID_ALARM);
        return;
    }

    if (pcvt->ista == menuCvtInitStateInProgress
        || pcvt->ista == menuCvtInitStateAgain) {
        recGblSetSevr(pcvt, SOFT_ALARM, MINOR_ALARM);
    }
    else if (pcvt->ista == menuCvtInitStateError) {
        recGblSetSevr(pcvt, SOFT_ALARM, MAJOR_ALARM);
    }

    hihi = pcvt->hihi;
    lolo = pcvt->lolo;
    high = pcvt->high;
    low = pcvt->low;
    hhsv = pcvt->hhsv;
    llsv = pcvt->llsv;
    hsv = pcvt->hsv;
    lsv = pcvt->lsv;
    val = pcvt->val;
    hyst = pcvt->hyst;
    lalm = pcvt->lalm;

    /* alarm condition hihi */
    if (hhsv && (val >= hihi || ((lalm == hihi) && (val >= hihi - hyst)))) {
        if (recGblSetSevr(pcvt, HIHI_ALARM, pcvt->hhsv))
            pcvt->lalm = hihi;
        return;
    }

    /* alarm condition lolo */
    if (llsv && (val <= lolo || ((lalm == lolo) && (val <= lolo + hyst)))) {
        if (recGblSetSevr(pcvt, LOLO_ALARM, pcvt->llsv))
            pcvt->lalm = lolo;
        return;
    }

    /* alarm condition high */
    if (hsv && (val >= high || ((lalm == high) && (val >= high - hyst)))) {
        if (recGblSetSevr(pcvt, HIGH_ALARM, pcvt->hsv))
            pcvt->lalm = high;
        return;
    }

    /* alarm condition low */
    if (lsv && (val <= low || ((lalm == low) && (val <= low + hyst)))) {
        if (recGblSetSevr(pcvt, LOW_ALARM, pcvt->lsv))
            pcvt->lalm = low;
        return;
    }

    /* we get here only if val is out of alarm by at least hyst */
    pcvt->lalm = val;
}

static long readInputs(struct cvtRecord *pcvt)
{
    long status;
    double old;

    old = pcvt->x;
    status = dbGetLink(&pcvt->inpx, DBR_DOUBLE, &pcvt->x, 0, 0);
    if (status) {
        recGblSetSevr(pcvt, LINK_ALARM, INVALID_ALARM);
        return status;
    }
    if (old != pcvt->x) {
        pcvt->drty |= DRTY_X;
    }

    old = pcvt->y;
    status = dbGetLink(&pcvt->inpy, DBR_DOUBLE, &pcvt->y, 0, 0);
    if (status) {
        recGblSetSevr(pcvt, LINK_ALARM, INVALID_ALARM);
        return status;
    }
    if (old != pcvt->y) {
        pcvt->drty |= DRTY_Y;
    }

    status = dbGetLink(&pcvt->iaml, DBR_ENUM, &pcvt->iaom, 0, 0);
    if (status) {
        recGblSetSevr(pcvt, LINK_ALARM, INVALID_ALARM);
        return status;
    }

    if (pcvt->iaom) {
        status = dbGetLink(&pcvt->iavl, DBR_DOUBLE, &pcvt->iaov, 0, 0);
        if (status) {
            recGblSetSevr(pcvt, LINK_ALARM, INVALID_ALARM);
            return status;
        }
        pcvt->val = pcvt->iaov;
    }

    return 0;
}

static long initConversion(
    const char *name, const char *bdir, const char *tdir,
    menuCvtMethod meth, const char *spec, void **psub, struct cvtRecord *pcvt)
{
    *psub = 0;
    switch (meth) {
        case menuCvtMethodLinear:
        {
            break;
        }
        case menuCvtMethodUser1DTableSub:
        {
          double *x_arr;
          double *y_arr;
          int no_of_elements;

          csm_function *csub; // dummy csub
          char temp[BDIR_SIZE+TDIR_SIZE+SPEC_SIZE+2];
          sprintf(temp, "%s/%s/%s", bdir, tdir, spec);

          csub = csm_new_function();

          // contained within csub is an array of coordinate structs which need
          // to be pulled out and given to the spline initialiser
          if (!csm_read_1d_table(temp, csub)) {
            nerrmsg(pcvt->name, "configuration error: "
                    "File %s is not a valid 1-parameter table", temp);
            csm_free(csub);
            return -1;
          }
          else {
            no_of_elements = get_arr_len(csub);
            x_arr = (double *)malloc(sizeof(double) * no_of_elements);
            y_arr = (double *)malloc(sizeof(double) * no_of_elements);
            // extract relevant data from csub
            x_arr = get_arr_values(csub, 'x', x_arr, no_of_elements);
            y_arr = get_arr_values(csub, 'y', y_arr, no_of_elements);
            // discard csub - no longer needed
            csm_free(csub);
          }
          //   Register function
          user1DTableSub_t user1DTableSub;

          user1DTableSub = (user1DTableSub_t) registryFunctionFind("user1DTableSub"); // string must match function supplied in support directory
          if (!user1DTableSub) {
              nerrmsg(name, "configuration error: subroutine not registered");
              return -1;
          }
          // Once read, run the function with an init flag to get it
          // to build a spline fit with the data read from the table.
          (*user1DTableSub)(true, x_arr, y_arr, no_of_elements, 0, &pcvt->dpvt);
          pcvt->csub = user1DTableSub;
          free(x_arr);
          free(y_arr);
          break;

        }
        case menuCvtMethodSubroutine:
        {
            REGISTRYFUNCTION csub;

            if(spec[0]==0) {
                nerrmsg(pcvt->name, "configuration error: SPEC not specified");
                return -1;
            }
            csub = registryFunctionFind(spec);
            if (!csub) {
                nerrmsg(pcvt->name, "configuration error: "
                    "SPEC is not the name of a registered subroutine");
                return -1;
            }
            *psub = csub;
            break;
        }
        case menuCvtMethod1DTable:
        case menuCvtMethod1DTableInverted:
        {
            csm_function *csub;
            char temp[BDIR_SIZE+TDIR_SIZE+SPEC_SIZE+2];

            csub = csm_new_function();
            if (!csub) {
                nerrmsg(pcvt->name, "csm_new_function failed");
                return -1;
            }
            sprintf(temp, "%s/%s/%s", bdir, tdir, spec);
            if (!csm_read_1d_table(temp, csub)) {
                nerrmsg(pcvt->name, "configuration error: "
                    "File %s is not a valid 1-parameter table", temp);
                csm_free(csub);
                return -1;
            }
            *psub = csub;
            break;
        }
        case menuCvtMethod2DTable:
        {
            csm_function *csub;
            char temp[BDIR_SIZE+TDIR_SIZE+SPEC_SIZE+2];

            csub = csm_new_function();
            if (!csub) {
                nerrmsg(pcvt->name, "csm_new_function failed");
                return -1;
            }
            sprintf(temp, "%s/%s/%s", bdir, tdir, spec);
            if (!csm_read_2d_table(temp, csub)) {
                nerrmsg(pcvt->name, "configuration error: "
                    "File %s is not a valid 2-parameter table", temp);
                csm_free(csub);
                return -1;
            }
            *psub = csub;
            break;
        }
    }
    return 0;
}

static long convert(struct cvtRecord *pcvt)
{
    double value;

    if (pcvt->iaom) {
        value = pcvt->iaov;
    } else {
        switch (pcvt->meth) {
            case menuCvtMethodLinear: {
                value = pcvt->x * pcvt->xslo + pcvt->y * pcvt->yslo + pcvt->voff;
                break;
            }
            case menuCvtMethodUser1DTableSub: {
                // call subroutine with false init flag so that it doesn't
                // try to rebuild the fit and realloc.
                user1DTableSub_t user1DTableSubPtr = (user1DTableSub_t)pcvt->csub;

                if (!user1DTableSubPtr) {
                  goto error;
                }
                value = (*user1DTableSubPtr)(false, 0, 0, 0, pcvt->x, &pcvt->dpvt);
                break;
            }
            case menuCvtMethodSubroutine: {
                cvt_subroutine *csub = (cvt_subroutine *)pcvt->csub;
                if (!csub) {
                    goto error;
                }
                value = csub(pcvt->x, pcvt->y, &pcvt->dpvt);
                break;
            }
            case menuCvtMethod1DTable: {
                csm_function *csub = (csm_function *)pcvt->csub;
                if (!csub) {
                    goto error;
                }
                value = csm_y(csub, pcvt->x);
                break;
            }
            case menuCvtMethod1DTableInverted: {
                csm_function *csub = (csm_function *)pcvt->csub;
                if (!csub) {
                    goto error;
                }
                value = csm_x(csub, pcvt->y);
                break;
            }
            case menuCvtMethod2DTable: {
                csm_function *csub = (csm_function *)pcvt->csub;
                if (!csub) {
                    goto error;
                }
                value = csm_z(csub, pcvt->x, pcvt->y);
                break;
            }
            default: {
                errmsg("internal error: METH is not a member of menuCvtMethod");
                goto error;
            }
        }
    }

    /* check drive limits */
    if (pcvt->drvh > pcvt->drvl) {
        if (value > pcvt->drvh)
            value = pcvt->drvh;
        else if (value < pcvt->drvl)
            value = pcvt->drvl;
    }

    pcvt->val = value;
    pcvt->udf = FALSE;
    return 0;

error:
    recGblSetSevr(pcvt, SOFT_ALARM, INVALID_ALARM);
    return -1;
}

static void monitor(struct cvtRecord *pcvt)
{
    unsigned short monitor_mask;
    double delta;

    monitor_mask = recGblResetAlarms(pcvt);
    /* check for value change */
    delta = pcvt->mlst - pcvt->val;
    if (delta < 0.0)
        delta = -delta;
    if (delta > pcvt->mdel) {
        /* post events for value change */
        monitor_mask |= DBE_VALUE;
        /* update last value monitored */
        pcvt->mlst = pcvt->val;
    }
    /* check for archive change */
    delta = pcvt->alst - pcvt->val;
    if (delta < 0.0)
        delta = -delta;
    if (delta > pcvt->adel) {
        /* post events on value field for archive change */
        monitor_mask |= DBE_LOG;
        /* update last archive value monitored */
        pcvt->alst = pcvt->val;
    }

    /* send out monitors connected to the value field */
    if (monitor_mask) {
        db_post_events(pcvt, &pcvt->val, monitor_mask);
    }
    if (pcvt->drty & DRTY_METH) {
        db_post_events(pcvt, &pcvt->meth, DBE_VALUE|DBE_LOG);
    }
    if (pcvt->drty & DRTY_SPEC) {
        db_post_events(pcvt, &pcvt->spec, DBE_VALUE|DBE_LOG);
    }
    if (pcvt->drty & DRTY_BDIR) {
        db_post_events(pcvt, &pcvt->bdir, DBE_VALUE|DBE_LOG);
    }
    if (pcvt->drty & DRTY_TDIR) {
        db_post_events(pcvt, &pcvt->tdir, DBE_VALUE|DBE_LOG);
    }
    if (pcvt->drty & DRTY_ISTA) {
        db_post_events(pcvt, &pcvt->ista, DBE_VALUE|DBE_LOG|DBE_ALARM);
    }
    if (pcvt->drty & DRTY_X) {
        db_post_events(pcvt, &pcvt->x, DBE_VALUE|DBE_LOG);
    }
    if (pcvt->drty & DRTY_Y) {
        db_post_events(pcvt, &pcvt->y, DBE_VALUE|DBE_LOG);
    }
    pcvt->drty = DRTY_NONE;
    return;
}

struct reinitMsg {
    struct cvtRecord *record;
    char spec[SPEC_SIZE];
    char bdir[TDIR_SIZE];
    char tdir[TDIR_SIZE];
    menuCvtMethod meth;
};

#define REINIT_MSG_SIZE sizeof(struct reinitMsg)

static long reinitConversion(struct cvtRecord *pcvt)
{
    long qstatus;
    struct reinitMsg msg;

    msg.record = pcvt;
    msg.meth = pcvt->nmet;
    if (pcvt->nmet != menuCvtMethodLinear) {
        strncpy(msg.spec, pcvt->nspe, SPEC_SIZE);
        strncpy(msg.bdir, pcvt->nbdi, BDIR_SIZE);
        strncpy(msg.tdir, pcvt->ntdi, TDIR_SIZE);
    }
    qstatus = epicsMessageQueueSend(
        initConversionQ, (void*)&msg, REINIT_MSG_SIZE);
    if (qstatus == -1) {
        errmsg("internal error: msgQ overrun");
        return -1;
    }
    return 0;
}

static void initConversionTask(void* parm)
{
    int qstatus;
    void *sub;
    struct reinitMsg msg;
    struct cvtRecord *pcvt;
    long status;

    while (TRUE) {
        qstatus = epicsMessageQueueReceive(
            initConversionQ, (void*)&msg, REINIT_MSG_SIZE);
        if (qstatus == -1) {
            nerrmsg("", "msgQReceive failed");
            continue;
        }
        pcvt = msg.record;
        status = initConversion(pcvt->name, msg.bdir, msg.tdir, msg.meth, msg.spec, &sub, pcvt);
        dbScanLock((struct dbCommon *)pcvt);
        if (status && pcvt->ista != menuCvtInitStateAgain) {
            if (pcvt->ista != menuCvtInitStateError) {
                pcvt->ista = menuCvtInitStateError;
                pcvt->drty |= DRTY_ISTA;
            }
        }
        else {
            switch (pcvt->ista) {
            case menuCvtInitStateInProgress:
            case menuCvtInitStateError:
                pcvt->ista = menuCvtInitStateDone;
                pcvt->drty |= DRTY_ISTA;
                /* free old csub if it was a csm_function... */
                if (pcvt->meth == menuCvtMethod1DTable
                    || pcvt->meth == menuCvtMethod1DTableInverted
                    || pcvt->meth == menuCvtMethod2DTable) {
                    csm_function *csub = (csm_function *)pcvt->csub;
                    if (csub) {
                        /* check because it might have never been created */
                        csm_free(csub);
                    }
                }
                /* ...and write the new values back into the record */
                pcvt->meth = msg.meth;
                pcvt->drty |= DRTY_METH;
                strncpy(pcvt->spec, msg.spec, SPEC_SIZE);
                pcvt->drty |= DRTY_SPEC;
                strncpy(pcvt->bdir, msg.bdir, BDIR_SIZE);
                pcvt->drty |= DRTY_BDIR;
                strncpy(pcvt->tdir, msg.tdir, TDIR_SIZE);
                pcvt->drty |= DRTY_TDIR;
                pcvt->csub = sub;
                break;
            case menuCvtInitStateAgain:
                if (!status && sub && (
                    pcvt->meth == menuCvtMethod1DTable
                    || pcvt->meth == menuCvtMethod1DTableInverted
                    || pcvt->meth == menuCvtMethod2DTable)) {
                    csm_free((csm_function *)sub);
                }
                /* even if initConversion(...) above failed, we go here */
                if (reinitConversion(pcvt)) {
                    /* this is fatal */
                    pcvt->ista = menuCvtInitStateError;
                    pcvt->drty |= DRTY_ISTA;
                    pcvt->pact = TRUE;
                    break;
                }
                pcvt->ista = menuCvtInitStateInProgress;
                pcvt->drty |= DRTY_ISTA;
                break;
            case menuCvtInitStateDone:
                errmsg("internal error: unexpected "
                    "value <menuCvtInitStateDone> in field ISTA");
                pcvt->pact = TRUE;
                break;
            default:
                errmsg("internal error: ISTA is not a member of menuCvtMethod");
                pcvt->pact = TRUE;
            }
        }
        checkAlarms(pcvt);
        monitor(pcvt);
        dbScanUnlock((struct dbCommon *)pcvt);
    }
}

static int countCvtRecords(void)
{
    DBENTRY dbentry;
    extern DBBASE *pdbbase;
    int result = 0;
    long status;

    dbInitEntry(pdbbase, &dbentry);
    status = dbFindRecordType(&dbentry,"cvt");
    if (!status) {
        result = dbGetNRecords(&dbentry);
    }
    dbFinishEntry(&dbentry);
    return result;
}

static long initialize()
{
    epicsThreadId tid;
    unsigned long cvtRecCnt = countCvtRecords();

    if (!initConversionQ && cvtRecCnt > 0) {
        initConversionQ = epicsMessageQueueCreate((unsigned)cvtRecCnt,
            REINIT_MSG_SIZE);
        if (!initConversionQ) {
            nerrmsg("", "msgQCreate failed");
            goto error;
        }
        tid = epicsThreadCreate("initCvt", epicsThreadPriorityLow, 20000,
            (EPICSTHREADFUNC)initConversionTask, 0);
        if (!tid) {
            nerrmsg("", "taskSpawn failed");
            goto error;
        }
    }
    return 0;

error:
    if (initConversionQ) epicsMessageQueueDestroy(initConversionQ);
    return -1;
}
