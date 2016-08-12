cvt - The Convert Record
========================


Introduction
============

The normal use for this record type is to convert one or two floating point
values into a resulting floating point value which is then forwarded
through the record's OUT link. It contains features from `ai`_, `ao`_, and
`sub`_ record types. The conversion can be specified according to four
different methods (linear, subroutine, 1-dimensional and 2-dimensional
tables) which themselves can be fully specified and also changed and
re-initialized at runtime. This record type has no device support. The
fields fall into the following categories:

-  `Scan Parameters`_
-  `Input and Output Parameters`_
-  `Conversion Parameters`_
-  `Operator Display Parameters`_
-  `Alarm Parameters`_
-  `Monitor Parameters`_
-  `Inactive Mode Parameters`_


Scan Parameters
===============

The cvt record has the standard fields for specifying under what
circumstances the record will be processed. See the
`Record Reference Manual, Scan Fields`_.
In addition, `Record Reference Manual, Scanning Specification`_,
explains how these fields are used. Note that I/O
event scanning is not supported.


Input and Output Parameters
===========================

The convert record has two input links (INPX, INPY) to specify where the
values to be converted should originate. In addition, the input values can
be written directly into the record's fields X and Y. The input values are
then converted, for details see `Conversion Parameters`_. The result value
is written into the VAL field. The VAL field's value is forced to be within
the limits specified in the fields DRVH and DRVL, which are configured by
the designer:

   DRVL ≤ VAL ≤ DRVH

It follows that if nothing is entered for DRVH and DRVL, the output value
will never change.

If the address contained in the output link (see `Conversion Parameters`_)
is a channel access or database link, the value in VAL is sent to the
address in the OUT field.

See `Record Reference Manual, Address Specification`_, for information on
how to specify links. `Record Reference Manual, Scanning Specification`_,
explains the effect of database linkage on scanning.

===== ================= ======== === ======= ====== ====== ================ ===
Field Summary           Type     DCT Initial Access Modify Rec Proc Monitor PP
===== ================= ======== === ======= ====== ====== ================ ===
X     Input Value X     DOUBLE   No    0     Yes      Yes       Yes         Yes
Y     Input Value Y     DOUBLE   No    0     Yes      Yes       Yes         Yes
VAL   Output Value      DOUBLE   No    0     Yes      No        Yes         Yes
INPX  Input X Location  INLINK   Yes         Yes      Yes       No          No
INPY  Input Y Location  INLINK   Yes         Yes      Yes       No          No
OUT   Output Link       OUTLINK  Yes         Yes      Yes       No          No
DRVH  Drive High        DOUBLE   Yes   0     Yes      Yes       No          Yes
DRVL  Drive Low         DOUBLE   Yes   0     Yes      Yes       No          Yes
===== ================= ======== === ======= ====== ====== ================ ===


Conversion Parameters
=====================

The field METH, of type `menuCvtMethod`_, specifies the conversion method:

-  ``LINEAR``: Linear conversion with two inputs::

      VAL = XSLO * X + YSLO * Y + VOFF

-  ``SUBROUTINE``: Custom conversion via an arbitrary subroutine whose
   name is given in SPEC. It is called with three arguments: The input
   values X and Y, and the address of the record's DPVT field. It should
   return the result value of the conversion. Symbolically::

      VAL = SPEC(X, Y, &DPVT)

   The subroutine may use the third parameter as it sees fit, which
   includes writing any pointer-sized value to the given address.
-  ``1D TABLE``: Table-driven conversion with one input value. SPEC
   should be the filename of a one-dimensional table as specified by the
   `csm module`_. The BDIR and TDIR field specify parts of the directory
   of the file, the complete path of the table file is BDIR/TDIR/SPEC.
   X is used as input to the csm routine `csm_y`_. The value of Y is ignored.
-  ``1D TABLE INVERTED``: Same as ``1D TABLE``, except conversion
   direction is backwards: Input is Y and conversion uses the csm routine
   `csm_x`_. The value of X is ignored.
-  ``2D TABLE``: Table-driven conversion with two input values. SPEC
   should be the filename of a two-dimensional table as specified by the
   `csm module`_. BDIR and TDIR togetehr determine the directory of the file,
   see case ``1D TABLE`` above. Both X and Y are used as inputs
   to the csm routine `csm_z`_.

The fields METH, SPEC, BDIR, and TDIR are read-only and always represent the
conversion currently in effect.

CVSR is used to implement the non-``LINEAR`` conversions.

===== ========================== ================= === ======= ====== ====== ================ ===
Field Summary                    Type              DCT Initial Access Modify Rec Proc Monitor PP
===== ========================== ================= === ======= ====== ====== ================ ===
METH  Conversion Method          `menuCvtMethod`_  Yes LINEAR  Yes      No    Yes             No
SPEC  Conversion Specification   STRING[40]        Yes         Yes      No    Yes             No
BDIR  Base Directory             STRING[40]        Yes         Yes      No    No              No
TDIR  Table Directory            STRING[40]        Yes         Yes      No    No              No
XSLO  Slope in X direction       DOUBLE            Yes 0       Yes      Yes   No              Yes
YSLO  Slope in Y direction       DOUBLE            Yes 0       Yes      Yes   No              Yes
VOFF  Value Offset               DOUBLE            Yes 0       Yes      Yes   No              Yes
CVSR  Conversion Subroutine      void*             No          No       No    No              No
===== ========================== ================= === ======= ====== ====== ================ ===


Conversion Re-Initialization
----------------------------

The method and other parameters of conversion can be changed at runtime
without interrupting operation. If conversion method is ``LINEAR``, new
values written to XSLO, YSLO, or VOFF will take immediate effect. For other
conversion types, or if the conversion type itself needs to be changed, new
conversion type values should be written to NMET and new conversion
specification strings to NSPE, new table directory parts to NBDI and NTDI.
These new values will not take immediate effect. Instead, conversion will be
re-initialized only after writing a non- zero value to the field INIT or if
the value retrieved by the input link INIL is non-zero.

===== ========================== ==================== === ======= ====== ====== ================ ===
Field Summary                    Type                 DCT Initial Access Modify Rec Proc Monitor PP
===== ========================== ==================== === ======= ====== ====== ================ ===
NMET  New Conversion Method      `menuCvtMethod`_     No  LINEAR  Yes      Yes      Yes          No
NBDI  New Base Directory         STRING[40]           No          Yes      Yes      No           No
NTDI  New Table Directory        STRING[40]           No          Yes      Yes      No           No
NSPE  New Conversion             STRING[40]           No          Yes      Yes      No           No
      Specification
ISTA  Initialization State       `menuCvtInitState`_  No  Done    Yes      No       Yes          No
INIT  Re-Initialize Conversion   UCHAR                No  0       Yes      Yes      No           No
INIL  Re-Init Conversion         INLINK               Yes         Yes      Yes      No           No
      Location
===== ========================== ==================== === ======= ====== ====== ================ ===

During re-initialization, the old conversion method and parameters remain
effective and record processing proceeds in the normal way. Field ISTA
contains the status of the last (re-)initialization attempt. STAT and SEVR of
the record are adjusted according to ISTA:

=========== ============================================ ======== ========
Value       Description                                  Status   Severity
=========== ============================================ ======== ========
Done        Initialization was successful                NO_ALARM NO_ALARM
InProgress  Initialization is in progress but not yet    SOFT     MINOR
            completed
Again       Initialization is in progress but must be    SOFT     MINOR
            done again after completion
Error       Initialization was aborted due to some error SOFT     MAJOR
=========== ============================================ ======== ========


Operator Display Parameters
===========================

These parameters are used to present meaningful data to the operator. They
display the value and other parameters of the analog output either textually
or graphically.

EGU is a string of up to 16 characters describing the units that the analog
output measures. It is retrieved by the get_units record support routine.

The HOPR and LOPR fields set the upper and lower display limits for the VAL,
OVAL, PVAL, HIHI, HIGH, LOW, and LOLO fields. Both the get_graphic_double and
get_control_double record support routines retrieve these fields. If these
values are defined, they must be in the range: DRVL<=LOPR<=HOPR<=DRVH.

The PREC field determines the floating point precision with which to display
VAL, X, and Y. It is used whenever the get_precision record support routine
is called.

See the `Record Reference Manual, Fields Common to All Record Types`_,
for more on the record name (NAME) and description (DESC) fields.

===== ==================== =========== === ======= ====== ====== ================ ===
Field Summary              Type        DCT Initial Access Modify Rec Proc Monitor PP
===== ==================== =========== === ======= ====== ====== ================ ===
EGU   Engineering Units    STRING [16] Yes          Yes     Yes   No              No
HOPR  High Operating Range DOUBLE      Yes   0      Yes     Yes   No              No
LOPR  Low Operating Range  DOUBLE      Yes   0      Yes     Yes   No              No
PREC  Display Precision    SHORT       Yes   0      Yes     Yes   No              No
NAME  Record Name          STRING [29] Yes          Yes     No    No              No
DESC  Description          STRING [29] Yes          Yes     Yes   No              No
===== ==================== =========== === ======= ====== ====== ================ ===


Alarm Parameters
================

The possible alarm conditions for analog outputs are the SCAN, READ, INVALID
and limit alarms. The SCAN, READ, and INVALID alarms are called by the record
or device support routines.

The limit alarms are configured by the user in the HIHI, LOLO, HIGH, and LOW
fields, which must be floating-point values. For each of these fields, there
is a corresponding severity field which can be either NO_ALARM, MINOR, or
MAJOR.

See `Record Reference Manual, Alarm Specification`_,
for a complete explanation of alarms and these fields.
See `Record Reference Manual, Invalid Alarm Output Action`_,
for more information on the IVOA
and IVOV fields. `Record Reference Manual, Alarm Fields`_,
lists other fields
related to a alarms that are common to all record types.

===== ==================== ================= === ======= ====== ====== ================ ===
Field Summary              Type              DCT Initial Access Modify Rec Proc Monitor PP
===== ==================== ================= === ======= ====== ====== ================ ===
HIHI  Hihi Alarm Limit     DOUBLE            Yes   0     Yes      Yes   No              Yes
HIGH  High Alarm Limit     DOUBLE            Yes   0     Yes      Yes   No              Yes
LOW   Low Alarm Limit      DOUBLE            Yes   0     Yes      Yes   No              Yes
LOLO  Lolo Alarm Limit     DOUBLE            Yes   0     Yes      Yes   No              Yes
HHSV  Hihi Alarm Severity  `menuAlarmSevr`_  Yes   0     Yes      Yes   No              Yes
HSV   High Alarm Severity  `menuAlarmSevr`_  Yes   0     Yes      Yes   No              Yes
LSV   Low Alarm Severity   `menuAlarmSevr`_  Yes   0     Yes      Yes   No              Yes
LLSV  Lolo Alarm Severity  `menuAlarmSevr`_  Yes   0     Yes      Yes   No              Yes
HYST  Alarm Deadband       DOUBLE            Yes   0     Yes      Yes   No              No
IVOA  Invalid Alarm Output `menuIvoa`_       Yes   0     Yes      Yes   No              No
      Action
IVOV  Invalid Alarm Output DOUBLE            Yes   0     Yes      Yes   No              No
      Value
===== ==================== ================= === ======= ====== ====== ================ ===


Monitor Parameters
==================

The fields ADEL and MDEL are used to specify deadbands for monitors on the
VAL field. The monitors are sent when the value field exceeds the last
monitored field by the specified deadband. If these fields have a value of
zero, everytime the value changes, a monitor will be triggered; if they have
a value of -1, everytime the record is processed, monitors are triggered.
ADEL is the deadband for archive monitors, and MDEL the deadband for all
other types of monitors. See `Record Reference Manual, Monitor Specification`_,
for a complete explanation of monitors.

The LALM, MLST, and ALST fields are used to implement the hysteresis factors
for monitor callbacks on VAL.

The DRTY field is used to implement monitors for the fields METH, SPEC, and
ISTA.

===== ======================= ======== === ======= ====== ====== ================ ===
Field Summary                 Type     DCT Initial Access Modify Rec Proc Monitor PP
===== ======================= ======== === ======= ====== ====== ================ ===
ADEL  Archive Deadband        DOUBLE   Yes   0     Yes      Yes   No              No
MDEL  Monitor Deadband        DOUBLE   Yes   0     Yes      Yes   No              No
LALM  Last Alarm Monitor      DOUBLE   No    0     Yes      No    No              No
      Trigger Value
ALST  Last Archiver Monitor   DOUBLE   No    0     Yes      No    No              No
      Trigger Value
MLST  Last Value Change       DOUBLE   No    0     Yes      No    No              No
      Monitor Trigger Value
DRTY  Dirty Bits (internal)   UCHAR    No    0     No       No    No              No
===== ======================= ======== === ======= ====== ====== ================ ===


Inactive Mode Parameters
========================

The convert record is either in active or inactive mode, depending on the
IAOM field. If the value is non-zero, the record will be inactive, otherwise
it will be active. The IAML link field can be used to input this value. An
inactive record does not perform any conversion. Instead VAL is set to the
value of the IAOV field. The IAVL link field can be used to input this value.

===== ======================= ============== === ======= ====== ====== ================ ===
Field Summary                 Type           DCT Initial Access Modify Rec Proc Monitor PP
===== ======================= ============== === ======= ====== ====== ================ ===
IAML  Inactive Mode Location  INLINK         Yes         Yes      Yes   No              No
IAOM  Inactive Mode           `menuYesNo`_   Yes   0     Yes      Yes   No              Yes
IAVL  Inactive Value Location INLINK         Yes         Yes      Yes   No              No
IAOV  Inactive Value          DOUBLE         Yes   0     Yes      Yes   No              Yes
===== ======================= ============== === ======= ====== ====== ================ ===


Record Support Routines
=======================

The following are the record support routines that would be of interest to an
application developer. Other routines are the ``get_units``,
``get_precision``, ``get_graphic_double``, and ``get_control_double``
routines.

init_record
-----------

This routine does nothing in pass 0. In pass 1, it does following:

-  If INPX and INPY are constant links, fields X and Y are initialized
   to the respective values.
-  NMET is set to METH, NSPE to SPEC, NBDI to BDIR, and NTDI to TDIR.
-  Conversion is initialized according to METH and SPEC. If an error
   occurs, conversion method falls back to ``LINEAR``.

process
-------

See `Record Processing`_.

special
-------

This routine is invoked whenever the field INIT is changed. If INIT was
changed to a non-zero value, then

-  Set INIT to zero.
-  If ISTA is ``Done`` or ``Error``, set ISTA to ``InProgress`` and
   start re-initialization.
-  If ISTA is ``InProgress``, set ISTA to ``Again``.

get_value
---------

Fills in the values of struct valueDes so that they refer to VAL.

get_alarm_double
----------------

Sets the following values:

-  upper_alarm_limit = HIHI
-  upper_warning_limit = HIGH
-  lower_warning_limit = LOW
-  lower_alarm_limit = LOLO


Record Processing
=================

Routine process implements the following algorithm:

1. Set PACT to 1.

2. Get value of INIT from input link INIL and proceed as in `special`_.

3. Read inputs:

   -  Get values of X and Y from INPX and INPY.
   -  Get inactive mode IAOM from IAML.
   -  If inactive, get inactive value IAOV from IAVL.

4. Convert and write result to VAL.

5. Check alarms: This routine checks to see if either the UDF field, the
   ISTA field, or the new VAL causes the alarm status and severity to
   change. If so, NSEV, NSTA and y are set. For status and severity changes
   caused by VAL exeeding alarm limits, the alarm hysteresis factor HYST is
   also honored. Thus the value must change by at least HYST before the
   alarm status and severity is reduced.

6. Check severity and write the new value. See
   `Record Reference Manual, Invalid Alarm Output Action`_,
   for details on how invalid alarms affect output records.

7. Check to see if monitors should be invoked:

   -  Alarm monitors are invoked if the alarm status or severity has
      changed.
   -  Archive and value change monitors are invoked if ADEL and MDEL
      conditions are met.
   -  Monitors for RVAL and for RBV are checked whenever other monitors
      are invoked.
   -  NSEV and NSTA are reset to 0.

8. Scan forward link if necessary, set PACT to 0, and return.


Menu Definitions
================


menuCvtMethod
-------------

============================= ==================
C Identifier                  Choice Name
============================= ==================
menuCvtMethodLinear           LINEAR
menuCvtMethodSubroutine       SUBROUTINE
menuCvtMethod1DTable          1D TABLE
menuCvtMethod1DTableInverted  1D TABLE INVERTED
menuCvtMethod2DTable          2D TABLE
============================= ==================

menuCvtInitState
----------------

============================= ==================
C Identifier                  Choice Name
============================= ==================
menuCvtInitStateDone          Done
menuCvtInitStateInProgress    InProgress
menuCvtInitStateAgain         Again
menuCvtInitStateError         Error
============================= ==================


.. _Record Reference Manual, Address Specification: http://www.aps.anl.gov/epics/wiki/index.php/RRM_3-14_Concepts#Address_Specification
.. _Record Reference Manual, Alarm Fields: http://www.aps.anl.gov/epics/wiki/index.php/RRM_3-14_dbCommon#Alarm_Fields
.. _Record Reference Manual, Alarm Specification: http://www.aps.anl.gov/epics/wiki/index.php/RRM_3-14_Concepts#Alarm_Specification
.. _Record Reference Manual, Fields Common to All Record Types: http://www.aps.anl.gov/epics/wiki/index.php/RRM_3-14_dbCommon#Miscellaneous_Fields
.. _Record Reference Manual, Invalid Alarm Output Action: http://www.aps.anl.gov/epics/wiki/index.php/RRM_3-14_Common#Invalid_Alarm_Output_Action
.. _Record Reference Manual, Monitor Specification: http://www.aps.anl.gov/epics/wiki/index.php/RRM_3-14_Concepts#Monitor_Specification
.. _Record Reference Manual, Scan Fields: http://www.aps.anl.gov/epics/wiki/index.php/RRM_3-14_dbCommon#Scan_Fields
.. _Record Reference Manual, Scanning Specification: http://www.aps.anl.gov/epics/wiki/index.php/RRM_3-14_Concepts#Scanning_Specification
.. _ai: http://www.aps.anl.gov/epics/wiki/index.php/RRM_3-14_Analog_Input
.. _ao: http://www.aps.anl.gov/epics/wiki/index.php/RRM_3-14_Analog_Output
.. _csm module: csmApp/html/index.html
.. _csm_x: csmApp/html/csmbase_8c.html#6226f2df9d594321101657cd5c53bb7d
.. _csm_y: csmApp/html/csmbase_8c.html#c28ee80fa3bcc8174ff0844ff92e981f
.. _csm_z: csmApp/html/csmbase_8c.html#c0e3dcd535ce486f004128f9c270cb2b
.. _menuAlarmSevr: http://www.aps.anl.gov/epics/wiki/index.php/RRM_3-14_Menu_Choices#menuAlarmSevr
.. _menuIvoa: http://www.aps.anl.gov/epics/wiki/index.php/RRM_3-14_Menu_Choices#menuIvoa
.. _menuYesNo: http://www.aps.anl.gov/epics/wiki/index.php/RRM_3-14_Menu_Choices#menuYesNo
.. _sub: http://www.aps.anl.gov/epics/wiki/index.php/RRM_3-14_Subroutine
