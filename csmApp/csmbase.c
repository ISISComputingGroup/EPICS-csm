/**************************************************************/
/*                           IDCP                             */
/*            The Insertion Device Control Program            */
/*                 Author: Götz Pfeiffer                     */
/**************************************************************/

/*____________________________________________________________*/
/* project information:                                       */
/*____________________________________________________________*/
/* project: IDCP                                              */
/* module: CSM-BASE                                           */
/* version-number of module: 1.1                              */
/* author: Götz Pfeiffer                                     */
/* last modification date: 2012-10-05                         */
/* status: tested                                             */
/*____________________________________________________________*/

/*____________________________________________________________*/
/*                          comments                          */
/*____________________________________________________________*/

    /*@ETI------------------------------------------------*/
    /*     comments for the automatically generated       */
    /*  header-file, this does not concern csm.c but      */
    /*                      csmbase.h !                   */
    /*----------------------------------------------------*/

/*@EM("/@ This file was automatically generated, all manual changes\n")
  @EM("   to this file will be lost, edit csm.c instead of csm.h !@/\n\n") */

    /*@ETI------------------------------------------------*/
    /*			    History                       */
    /*----------------------------------------------------*/

/*
Version 0.9:
  Date: 6/16/97
  This is the first implementation csm.

Version 0.91:
  Date: 8/22/97
  A mechanism to accelerate the search of x-values in the table has been
  implemented. Now the interval-borders from the previous search are
  tested (and if it makes sense taken as borders for the new search).
  This should speed up the process if table_x_lookup_ind is called with
  x-values that do not differ too much from each other (in terms of the
  table-values). This version has not yet been tested.

Version 0.92:
  Date: 10/16/97
  The program (csmbase) has been splitted from the original csm and now
  contains only generic routines for implementing linear and table-based
  conversions. All IDCP specific conversion funtions are now placed
  in csm.c. Note that for this file (csmbase.c) version 0.92 is the first
  version!

Version 0.93:
  Date: 5/5/99
  csmbase was extended in order to support 2-dimensional functions (z=f(x,y))
  These functions are specified in form of a table :
	 y1   y2   y3   .   .   .
     x1  z11  z12  z13  .   .   .
     x2  z21  z22  z23  .   .   .
     .    .    .    .
     .    .    .    .
     .    .    .    .

Version 0.94:
  Date: 6/16/99
  The functions csm_read_table and csm_read_xytable were changed in order to
  work even if some lines of the data-file do not contain data (this is
  especially the case with empty lines).

Version 0.95:
  Date: 5/26/00
  An error in coordinate_lookup_index() was removed. This caused the function
  to return wrong parameters when applied with x==2.

Version 0.96:
  Date: 10/13/00
  Small changes. If a function is not yet defined (type CSM_NOTHING), the
  function returns 0.

  Date: 2002-03-05
  Errors in the header-file generation were fixed, some
  left-overs from previous debugging were removed

  Date: 2004-04-05
  in csm_read_table a superfluous parameter was removed

  Date: 2004-07-23
  a new function, csm_clear was added. This function releases all
  memory an csm_function occupies and sets its value to CSM_NOTHING

  Date: 2004-08-31
  a new function, csm_free was added. This function releases all
  memory and finally releases the memory of the csm_function object
  itself.

  Date: 2006-03-01
  bugfix: semDelete was added

  Date: 2006-03-01
  * usage of psem (portable semaphores) can now be selected
  * usage of DBG, errlogPrinf or printf can now be selected
    define USE_DBG and USE_ERRLOGPRINTF accordingly
  * prepared for Linux (undefine B_VXWORKS for this,
    set USE_DBG 0, USE_ERRLOGPRINTF 0 and USE_PSEM 1
  * memory leak error was fixed in csm_read_2d_table

  Date: 2006-03-02
  cosmetic changes

  Date: 2006-03-02
  changes in the embedded documentation

  Date: 2007-11-14
  * bugfix with respect to the on-hold function: The "last" field
    of the csm_Function structure has to exist for each type of
    function that can be applied to the structure, one for
    csm_x(), one for csm_y(), one for csm_z(), for csm_dx() and
    csm_dy(). With only one field, it may happen for example, 
    that when "on_hold" is used, csm_x() returns the last returned 
    value of csm_y(). 
  * bugfix with respect to multithreading use:
    the semaphore used in functions like csm_x(), csm_y() and so
    on was given just before a return-statement which did a read-access
    on the function structure. This lead to a race condition where
    it was possible that one call of csm_x() returned the value of 
    another call of csm_x() which was called by another task. Together
    with "last" field bug mentioned above, csm_x() could even return
    the value of the last csm_y() call, which is fatal...
  * str_empty_or_comment() now accepts lines where "#" is not 
    in the first column.  
  * the inline documentation was updated  

  Date: 2007-11-14
  * the usage of semaphores can no be disabled by a macro.
    This was needed for better portability.

  Date: 2011-08-16
  * The functions lookup_1d_functiontable and lookup_2d_functiontable now
    return NAN (not a number) in case the table doesn't contain any values. 
  * lookup_1d_functiontable now handles the case of a table with a single
    point (xp,yp) correctly. It then returns the associated yp for any value of
    x. Before this change, it returned NAN for x!=xp and yp for x==xp.

  Date: 2012-04-25
  * The function coordinate_lookup_index was optimized. When a new value is
    looked up in the table it first tries to find it in the last interval, then
    in the surrounding intervals below and above the orginal one. Only if the
    value is not found there it does a binary search. 
  * The functions lookup_1d_functiontable and lookup_2d_functiontable were
    optimized. They now cache the last value that was looked up in the table.
    If they are called with the same value or values again, they return the
    cached value which is much faster than to re-run the calculations for the
    interpolation.
*/

    /*----------------------------------------------------*/
    /*		        General Comments                  */
    /*----------------------------------------------------*/

/*! \file csmbase.c
    \brief implement function table
    \author Götz Pfeiffer (goetz.pfeiffer\@helmholtz-berlin.de)
    \version 1.0

   This module implements one and two dimensional function tables.
   The tables can be specified by simple ascii files. The points
   defined in the file don't have to be in the same distance.
*/


/*@ITI________________________________________________________*/
/*                      general Defines                       */
/*@ET_________________________________________________________*/

/*@EM("\n#ifndef __CSMBASE_H\n#define __CSMBASE_H\n") */

#ifndef DOXYGEN_SKIP_THIS

#ifndef B_LINUX
/*! \internal \brief select compilation under vxWorks */
#define B_VXWORKS
#endif

/*! \internal \brief needed for POSIX compability */
#define _INCLUDE_POSIX_SOURCE

/*! \internal \brief use DBG module ? */
#define USE_DBG 0

/*! \internal \brief use epics errlogPrintf ? */
#define USE_ERRLOGPRINTF 1

/*! \internal \brief use semaphores at all ? */
#define USE_SEM 1

/*! \internal \brief use psem semaphore module ? */
#define USE_PSEM 0

/* the following macros affect the debug (dbg) module: */

#if USE_DBG

/*! \internal \brief compile DBG assertions */
#define DBG_ASRT_COMP 0

/*! \internal \brief compile DBG debug messages */
#define DBG_MSG_COMP 0

/*! \internal \brief compile DBG trace messages */
#define DBG_TRC_COMP  0

/*! \internal \brief use local switch-variables for the dbg module */
#define DBG_LOCAL_COMP 1

/*! \internal \brief use async. I/O with message passing in DBG module*/
#define DBG_ASYNC_COMP 1

/* the following defines are for sci-debugging, they do not influence
   any header-files but are placed here since they are usually changed
   together with the above macros.*/

/*! \internal \brief for debug-outputs, "stdout" means console */
#define CSMBASE_OUT_FILE "stdout"

/*! \internal \brief csm trace level

  \li level 6: dense traces for debugging
  \li level 5: enter/exit tracing
*/
#define CSMBASE_TRC_LEVEL 0

/*! \internal \brief flushmode, 0, 1 and 2 are allowed */
#define CSMBASE_FLUSHMODE 1

#else

#if !USE_ERRLOGPRINTF
#define errlogPrintf printf
#endif

/*! \internal \brief compability macro, needed when DBG is not used */
#define DBG_MSG_PRINTF2(f,x) errlogPrintf(f,x)

/*! \internal \brief compability macro, needed when DBG is not used */
#define DBG_MSG_PRINTF3(f,x,y) errlogPrintf(f,x,y)

/*! \internal \brief compability macro, needed when DBG is not used */
#define DBG_MSG_PRINTF4(f,x,y,z) errlogPrintf(f,x,y,z)

/*! \internal \brief compability macro, needed when DBG is not used */
#define DBG_MSG_PRINTF5(f,x,y,z,q) errlogPrintf(f,x,y,z,q)

#endif

#if !USE_SEM
#define SEM_TYPE int
#define SEMTAKE(x) 
#define SEMGIVE(x) 
#define SEMDELETE(x) 
#define SEMCREATE(x) 0 

#else

#if USE_PSEM
#define SEM_TYPE psem_mutex
#define SEMTAKE(x) psem_mutex_take(&(x))
#define SEMGIVE(x) psem_mutex_give(&(x))
#define SEMDELETE(x) psem_mutex_remove(&(x))
#define SEMCREATE(x) (!psem_mutex_create(&(x)))
#else
/*! \internal \brief compability macro for semaphores */
#define SEM_TYPE epicsMutexId
/*! \internal \brief compability macro for semaphores */
#define SEMTAKE(x) epicsMutexLock(x)
/*! \internal \brief compability macro for semaphores */
#define SEMGIVE(x) epicsMutexUnlock(x)
/*! \internal \brief compability macro for semaphores */
#define SEMDELETE(x) epicsMutexDestroy(x)
/*! \internal \brief compability macro for semaphores */
#define SEMCREATE(x) (NULL==(x= epicsMutexCreate()))
#endif

#endif

#endif

/*! \brief for type csm_bool */
#define CSM_TRUE 1

/*! \brief for type csm_bool */
#define CSM_FALSE 0

/*____________________________________________________________*/
/*			 Include-Files			      */
/*____________________________________________________________*/

#if USE_SEM

#if USE_PSEM
#include <psem.h>
#else
#include <epicsMutex.h>
#endif

#endif

#if USE_DBG
#include <dbg.h>
#endif

#if USE_ERRLOGPRINTF
#include <errlog.h> /* epics error printf */
#endif

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* math.h for definition of NAN */
#include <math.h>

/*@ITI________________________________________________________*/
/*                         Defines                            */
/*@ET_________________________________________________________*/

      /*................................................*/
      /*@IL           the NAN number type               */
      /*................................................*/

#ifndef NAN
/*! \internal \brief define NAN if it is not yet defined */
static double makeNAN()
{
    static double zero = 0.0;
    return -(zero / zero);
}
/* #define NAN (-(0.0/0.0)) */
#define NAN makeNAN()
#endif

/*@ITI________________________________________________________*/
/*                          Types                             */
/*@ET_________________________________________________________*/

      /*................................................*/
      /*@IL        the csm_bool type (public)           */
      /*................................................*/

/*! \brief the boolean data type used in this module */

typedef int csm_bool; /*@IL*/

      /*................................................*/
      /*          the coordinate type (private)         */
      /*................................................*/

/*! \internal \brief a single coordinate value */
typedef struct
  {
    double value; /*!< the floating point value of the coordinate */
    int    index; /*!< the index-number within this list of coordinates */
  } csm_coordinate;

/*! \internal \brief a list of coordinates */
typedef struct
  { csm_coordinate *coordinate;  /*!< pointer to coordinate array */
    int            no_of_elements;
                                 /*!< number of elements in the coordinate
				      array */
    int            a_last;       /*!< last left index */
    int            b_last;       /*!< last right index */
  } csm_coordinates;

      /*................................................*/
      /*        the linear function type (private)      */
      /*................................................*/

/*! \internal \brief a linear function y = a + b*x */
typedef struct
  { double a;         /*!< a in y= a+b*x */
    double b;         /*!< b in y= a+b*x */
  } csm_linear_function;

      /*................................................*/
      /*           the 1d table type (private)          */
      /*................................................*/

/*! \internal \brief one-dimenstional function table

  This is a list of x-y pairs that define a one-dimensional
  function. The function can be inverted, if it is monotone.
*/
typedef struct
  { csm_coordinates x;     /*!< list of x-coordinates */
    csm_coordinates y;     /*!< list of y-coordinates */
    double x_last;         /*!< last x value looked up */
    double y_last;         /*!< last y value looked up */
    csm_bool value_cached; /*!< 1 if x_last, y_last valid */
  } csm_1d_functiontable;

      /*................................................*/
      /*          the 2d table type (private)           */
      /*................................................*/

/*! \internal \brief two-dimenstional function table

  This is a list of x-y pairs and corresponding z-values that
  define a two-dimensional function table. The function cannot be inverted.
*/
typedef struct
  { csm_coordinates x;   /*!< list of x-coordinates */
    csm_coordinates y;   /*!< list of y-coordinates */
    double          *z;  /*!< list of z-values (function values) */
    double x_last;         /*!< last x value looked up */
    double y_last;         /*!< last y value looked up */
    double z_last;         /*!< last z value looked up */
    csm_bool value_cached; /*!< 1 if x_last, y_last valid */
  } csm_2d_functiontable;

      /*................................................*/
      /*           the function-type (private)          */
      /*................................................*/

#ifndef DOXYGEN_SKIP_THIS
/*! \internal \brief typedef-enum: types of functions known in this module */
typedef enum { CSM_NOTHING,  /*!< kind of NULL value */
               CSM_LINEAR,   /*!< linear y=a+b*x */
               CSM_1D_TABLE, /*!< one-dimensional function table */
	       CSM_2D_TABLE  /*!< two-dimensional function table */
	     } csm_func_type;
#endif

/*! \internal \brief compound type for functions */
struct csm_Function
  { SEM_TYPE semaphore;   /*!< semaphore to lock access to csm_function */
    double   last_x;      /*!< last x value that was calculated */
    double   last_y;      /*!< last y value that was calculated */
    double   last_dx;     /*!< last dx value that was calculated */
    double   last_dy;     /*!< last dy value that was calculated */
    double   last_z;      /*!< last z value that was calculated */
    csm_bool on_hold;     /*!< when TRUE, just return last */
    csm_func_type type;   /*!< the type of the function */

    union { csm_linear_function    lf;   /*!< linear function */
            csm_1d_functiontable   tf_1; /*!< 1d function table */
            csm_2d_functiontable   tf_2; /*!< 2d function table */
	  } f;            /*!< union that holds the actual data */

  };

      /*................................................*/
      /*@IL       the function-object (public)          */
      /*................................................*/

/*! \brief the abstract csm function object */
/*@IT*/
typedef struct csm_Function csm_function;
/*@ET*/

/*____________________________________________________________*/
/*			  Variables			      */
/*____________________________________________________________*/

      /*................................................*/
      /*           initialization (private)             */
      /*................................................*/

/*! \internal \brief initialization flag [static]

  This variable is used to remember, wether the module
  was already initialized.
*/
static csm_bool initialized= CSM_FALSE;

/*____________________________________________________________*/
/*                        Functions                           */
/*@ET_________________________________________________________*/


    /*----------------------------------------------------*/
    /*             debug - managment (private)            */
    /*----------------------------------------------------*/

#if USE_DBG
/*! \internal \brief initialize the DBG module [static]

  This internal function is called by csmbase_init
*/

static void csm_dbg_init(void)
  { static csm_bool csmbase_init= CSM_FALSE;

    if (csmbase_init)
      return;
    csmbase_init= CSM_TRUE;
    /* initialize the debug (dbg) module: use stdout for error-output */
    DBG_SET_OUT(CSMBASE_OUT_FILE);
    DBG_TRC_LEVEL= CSMBASE_TRC_LEVEL;
                           /* level 6: dense traces for debugging
                              level 5: enter/exit tracing
                              level 2: output less severe errors and warnings
                              level 1: output of severe errors
                              level 0: no output */
    DBG_FLUSHMODE(CSMBASE_FLUSHMODE);
  }

#endif

    /*----------------------------------------------------*/
    /*        utilities for csm-coordinates (private)     */
    /*----------------------------------------------------*/

        /*              initialization                */

/*! \internal \brief initialize a csm_coordinates structure [static]

  This function initializes a csm_coordinates structure. This function
  should be called immediately after a variable of the type
  csm_coordinates was created.
  \param c pointer to the csm_coordinates array
*/
static void init_coordinates(csm_coordinates *c)
  {
    c->a_last=-1;
    c->b_last=-1;
    c->coordinate= NULL;
    c->no_of_elements=0;
  }

/*! \internal \brief initialize a csm_coordinates structure [static]

  This function allocates memory for a
  csm_coordinates structure.
  \param c pointer to the csm_coordinates structure
  \param elements number of elements in the structure
  \return returns CSM_FALSE in case of an error, CSM_TRUE otherwise
*/
static csm_bool alloc_coordinates(csm_coordinates *c, int elements)
  { int i;
    csm_coordinate *co;

    if (c->no_of_elements==0) /* 1st time memory allocation */
      c->coordinate= malloc(sizeof(csm_coordinate)*elements);
    else
      c->coordinate= realloc(c->coordinate,
                             sizeof(csm_coordinate)*elements);

    if (c->coordinate==NULL) /* allocation error */
      { DBG_MSG_PRINTF2("error in csm:alloc_coordinates line %d,\n"
                	"allocation failed!\n", __LINE__);
	init_coordinates(c);
	return(CSM_FALSE);
      };

    for(i=c->no_of_elements, co= (c->coordinate + c->no_of_elements);
        i<elements;
	i++, co++)
      { co->value=0;
	co->index=i;
      };

    c->no_of_elements= elements;
    return(CSM_TRUE);
  }

/*! \internal \brief resize a csm_coordinates structure [static]

  This function resizes a csm_coordinates structure. This means that
  the size of allocated memory is adapted to a new, different number
  of elements. Note that the existing data is not changed. If additional
  memory is allocated, it is initialized with zeroes.
  \param c pointer to the csm_coordinates structure
  \param elements new number of elements in the structure
  \return returns CSM_FALSE in case of an error, CSM_TRUE otherwise
*/
static csm_bool resize_coordinates(csm_coordinates *c, int elements)
  { if (elements==c->no_of_elements)
      return(CSM_TRUE);

    c->no_of_elements= elements;
    if (NULL== (c->coordinate= realloc(c->coordinate,
                                       sizeof(csm_coordinate)*elements)))
      { DBG_MSG_PRINTF2("error in csm:resize_coordinates line %d,\n" \
                        "realloc failed!\n", __LINE__);
        return(CSM_FALSE);
      };
    return(CSM_TRUE);
  }

/*! \internal
    \brief re-initialize a csm_coordinates structure [static]

  This re-initializes a csm_coordinates structure. This function
  is similar to init_coordinates, but already allocated memory is
  freed before the structure is filled with it's default values.
  \param c pointer to the csm_coordinates array
*/
static void reinit_coordinates(csm_coordinates *c)
  { if (c->no_of_elements==0)
      return;

    free(c->coordinate);
    init_coordinates(c);
  }

/*! \internal \brief initialize a matrix of double values [static]

  This function allocates an array of doubles in a way that it
  has enough room to hold all values of a rows*columns matrix.
  \param z address of pointer to array of double-numbers (output)
  \param rows number of rows
  \param columns number of columns
  \return returns CSM_FALSE in case of an error, CSM_TRUE otherwise
*/
static csm_bool init_matrix(double **z, int rows, int columns)
  { int i;
    double *zp;

    if (NULL== (zp= malloc((sizeof(double))*rows*columns)))
      { DBG_MSG_PRINTF2("error in csm:init_matrix line %d,\n" \
                        "malloc failed!\n", __LINE__);
        return(CSM_FALSE);
      };
    *z = zp;
    for (i= rows*columns; i>0; i--, *(zp++)=0);
    return(CSM_TRUE);
  }

        /*            x-compare for qsort             */

/*! \internal
    \brief compare two values within a csm_coordinate structure [static]

  This function compares two values within a cms_coordinate structure.
  It is needed in order to apply quicksort.
  \param t1 this is the pointer to the first value. It should be of the
            type csm_coordinate*.
  \param t2 this is the pointer to the second value. It should be of the
            type csm_coordinate*.
  \return returns -1 if *t1 < *t2, 0 if *t1 = *t2, 1 if *t1 > *t2
*/
static int coordinate_cmp(const void *t1, const void *t2)
  { double x1= ((csm_coordinate *)(t1))->value;
    double x2= ((csm_coordinate *)(t2))->value;

    if (x1 < x2)
      return(-1);
    if (x1 > x2)
      return( 1);
    return(0);
  }

        /*              coordinate-sort               */

/*! \internal \brief sort coordinates in a csm_coordinates structure [static]

  This function sorts the coordinates within a
  csm_coordinates structure according to their value field.
  \param coords pointer to the csm_coordinates structure
*/
static void coordinate_sort( csm_coordinates *coords)
  {
    qsort( coords->coordinate, coords->no_of_elements,
           sizeof(csm_coordinate), coordinate_cmp);
  }

/*! \internal \brief update backlinks from on csm_coordinates structure to another [static]

  Two or more csm_coordinates structures can be connected by the
  index fields of their elements. The index field of each element
  in the first structure is then the index of the corresponding
  element in the second structure and vice versa. If the order
  of the element in one of the structures was changed, the index
  field in all elements of the second structure have to be updated,
  which is what this function does. It is usually called after
  sorting the first structure with coordinate_sort.
  \param coords1 pointer to the first csm_coordinates structure which
         was re-ordered.
  \param coords2 pointer to the second csm_coordinates structure which
         has to be updated.
*/
static void coordinate_update_backlinks( csm_coordinates *coords1,
				         csm_coordinates *coords2)
  { int i,l;
    csm_coordinate *c,*d;

    l= coords1->no_of_elements;
    for (i=0, c= coords1->coordinate, d= coords2->coordinate; i<l; i++, c++)
      { d[c->index].index = i; };
  }

        /*      lookup an index in coordinates        */

/*! \internal \brief lookup nearest boundaries within a csm_coordinates structure [static]

  This searches for the nearest boundaries to a given x within a given
  csm_coordinates structure. It looks up the indices of two
  elements. For the first one,it is guaranteed that its value is
  smaller or equal to x and that no other element with a larger value
  has this property. For the second one, it is guaranteed that its value
  is bigger or equal to x and that no other element with a smaller value
  has this property. so
     coords->coordinate[a].value <= x <= coords->coordinate[b].value
  holds. If a or b cannot be found since x is at the border of the
  list of coordinates, they are set to -1.
  \param coords pointer to the first csm_coordinates structure
  \param x the value to find
  \param a pointer to the returned index of the smaller element
  \param b pointer to the returned index of the bigger element
  \return 2 if x was found, a is the corresponding index
          1 x was not found, but a and b define the closest interval around x
          0 x was not found at it is outside the closest interval a and b
            a or b may be -1
         -1 error
*/

#define ret(idx_a, idx_b, ret_code) \
  *a= idx_a;\
  *b= idx_b;\
  coords->a_last= idx_a;\
  coords->b_last= idx_b;\
  return(ret_code)
  
static int coordinate_lookup_index(csm_coordinates *coords,double x,
				   int *a, int *b)
  /* 2: x was found, it is returned in a
     1: x was not found, but a and b define the closest interval around x
     0: x was not found at it is outside the closest interval a and b
    -1: error
  */
  /* the table should be x-sorted */
  { int i, ia, ib;
    double xt;
    csm_coordinate *c= coords->coordinate;
    double x_boundary;
    int last_idx= coords->no_of_elements-1;
    int a_last, b_last;
    int tries;

    if (coords->no_of_elements<2)
      { if (coords->no_of_elements<1)
          {
            DBG_MSG_PRINTF2("error in csm:coordinate_lookup_index %d,\n" \
                            "table has less than 1 element!\n", __LINE__);
            ret(0,0,-1);
           };
        coords->a_last= 0; coords->b_last= 0; 
	if (x== c[0].value)
          {
            ret(0,0,2);
          }
        else
          {
            ret(0,0,0);
          }
      };

    a_last= coords->a_last;
    b_last= coords->b_last;
    if (a_last<0)
      a_last= 0;
    if (b_last<0)
      b_last= last_idx;

    for(tries=1; tries<=3; tries++)
      {
        switch(tries)
          {
            case 1:
              break;
            case 2:
              b_last= a_last;
              a_last= a_last-1;
              break;
            case 3:
              b_last= a_last;
              a_last= 0;
              break;
          }

        x_boundary= c[a_last].value;
        if (x>x_boundary)
          break;
        if (x==x_boundary)
          {
            ret(a_last,a_last,2);
          }
        if (a_last==0)
          { /* left of area */
            ret(0,1, 0);
          }
      }

    for(tries=1; tries<=3; tries++)
      {
        switch(tries)
          {
            case 1:
              break;
            case 2:
              a_last= b_last;
              b_last= b_last+1;
              break;
            case 3:
              a_last= b_last;
              b_last= last_idx;
              break;
          }

        x_boundary= c[b_last].value;
        if (x<x_boundary)
          break;
        if (x==x_boundary)
          {
            ret(b_last,b_last,2);
          }
        if (b_last==last_idx)
          { /* right of area */
            ret(last_idx-1,last_idx, 0);
          }
      }

    ia= a_last;
    ib= b_last;
    /* necessary condition here:
       c[ia].value<= x <= c[ib].value
     */
    for (; (ib>ia+1); )
      { i= (ib+ia)/2;
        xt= c[i].value;
        if (xt<x)
          {
            ia= i;
            continue;
          }
        if (xt>x)
          {
            ib= i;
            continue;
          }
        /* exact match */
        ret(i,i,2);
      };
    ret(ia,ib,1);
  }

    /*----------------------------------------------------*/
    /*     utilities for csm_linear_function (private)    */
    /*----------------------------------------------------*/

        /*               calculation                  */

/*! \internal \brief compute y from a given x [static]

  This function computes y when x is given for a linear function.
  A linear function is a
  \param p pointer to the linear function structure
  \param x the value of x
*/
static double linear_get_y(csm_linear_function *p, double x)
  { return( (p->a) + (p->b)*x ); }

/*! \internal \brief compute x from a given y [static]

  This function computes x when y is given for a linear function
  \param p pointer to the linear function structure
  \param y the value of y
*/
static double linear_get_x(csm_linear_function *p, double y)
  { return( (y - (p->a))/(p->b) ); }

/*! \internal \brief compute delta-y from a given delta-x [static]

  This function computes delta-y when delta-x is given for a linear function
  \param p pointer to the linear function structure
  \param x the value of x
*/
static double linear_delta_get_y(csm_linear_function *p, double x)
  { return( (p->b)*x ); }

/*! \internal \brief compute delta-x from a given delta-y [static]

  This function computes delta-x when delta-y is given for a linear function
  \param p pointer to the linear function structure
  \param y the value of y
*/
static double linear_delta_get_x(csm_linear_function *p, double y)
  { return( y/(p->b) ); }

    /*----------------------------------------------------*/
    /*    utilities for csm_1d_functiontable (private)    */
    /*----------------------------------------------------*/

        /*              initialization                */

/*! \internal \brief initialize a csm_1d_functiontable structure [static]

  This function initializes all members of the csm_1d_functiontable
  structure. This function should be called immediately after a
  variable of the type csm_1d_functiontable was created.
  \param ft pointer to the csm_1d_functiontable structure
*/
static void init_1d_functiontable(csm_1d_functiontable *ft)
  { init_coordinates(&(ft->x));
    init_coordinates(&(ft->y));
    ft->value_cached= CSM_FALSE;
  }

/*! \internal \brief reinitialize a csm_1d_functiontable structure [static]

  This function re-initializes the csm_coordinates members of the
  csm_1d_functiontable structure. This is similar to init_1d_functiontable,
  but already allocated memory is freed before the structures is filled
  with their default values.
  \param ft pointer to the csm_1d_functiontable structure
*/
static void reinit_1d_functiontable(csm_1d_functiontable *ft)
  { reinit_coordinates(&(ft->x));
    reinit_coordinates(&(ft->y));
    ft->value_cached= CSM_FALSE;
  }

        /*        lookup a value in the table         */

/*! \internal \brief lookup a value in a csm_1d_functiontable structure [static]

  This function looks up the function value y for a given x in a
  csm_1d_functiontable structure which is a one-dimensional function
  table. The two points x1 and x2 that are
  nearest to x are searched. A linear interpolation between the
  corresponding y1 and y2 values is done and this value is returned.
  If inverted lookup is wanted. the function looks up y1 and y2 for a
  given y and does a linear interpolation between x1 and x2.
  \param ft pointer to the csm_1d_functiontable structure
  \param x the value to look up
  \param invert if CSM_TRUE, an inverted lookup is done
  \return the value that fits best to x is returned.
*/
static double lookup_1d_functiontable(csm_1d_functiontable *ft, double x,
				      csm_bool invert)
  { int res,a,b;
    double xa,xb,ya,yb;
    csm_coordinate c;
    csm_coordinates *xcoords;
    csm_coordinates *ycoords;

    if (!invert)
      { 
        if ((ft->value_cached) && (ft->x_last==x))
          return(ft->y_last);
        xcoords= &(ft->x);
        ycoords= &(ft->y);
      }
    else
      { 
        if ((ft->value_cached) && (ft->y_last==x))
          return(ft->x_last);
        xcoords= &(ft->y);
        ycoords= &(ft->x);
      };

    res= coordinate_lookup_index(xcoords, x, &a, &b);


    if (res==-1) /* error */
      {
        ft->value_cached= CSM_FALSE;
        return(NAN);
      }
    if ((res== 2)||(a==b)) /* exact match or table with just a single value */
      { 
        /* if the table has just a single value (only one [x,y] pair) we return
           the corresponding y value. */
        c= (xcoords->coordinate)[a];
        ft->value_cached= CSM_TRUE;
        if (!invert)
          {
            ft->x_last= x;
            return(ft->y_last= (ycoords->coordinate)[c.index].value);
          }
        else
          {
            ft->y_last= x;
            return(ft->x_last= (ycoords->coordinate)[c.index].value);
          }
      };

    c= (xcoords->coordinate)[a];
    xa= c.value;
    ya= (ycoords->coordinate)[c.index].value;

    c= (xcoords->coordinate)[b];
    xb= c.value;
    yb= (ycoords->coordinate)[c.index].value;

    ft->value_cached= CSM_TRUE;

    if (!invert)
      {
        ft->x_last= x;
        /* y= ya + (x-xa)/(xb-xa)*(yb-ya) */
        return(ft->y_last= ya + (x-xa)/(xb-xa) * (yb-ya));
      }
    else
      {
        ft->y_last= x;
        /* y= ya + (x-xa)/(xb-xa)*(yb-ya) */
        return(ft->x_last= ya + (x-xa)/(xb-xa) * (yb-ya));
      }

  }

    /*----------------------------------------------------*/
    /*    utilities for csm_2d_functiontable (private)    */
    /*----------------------------------------------------*/

        /*              initialization                */

/*! \internal \brief initialize a csm_2d_functiontable structure [static]

  This function initializes all members of the csm_2d_functiontable
  structure. This function should be called immediately after a
  variable of the type csm_2d_functiontable was created.
  \param ft pointer to the csm_2d_functiontable structure
*/
static void init_2d_functiontable(csm_2d_functiontable *ft)
  { init_coordinates(&(ft->x));
    init_coordinates(&(ft->y));
    ft->value_cached= CSM_FALSE;
  }

/*! \internal \brief reinitialize a csm_2d_functiontable structure [static]

  This function re-initializes the csm_coordinates members of the
  csm_2d_functiontable structure. This is similar to init_2d_functiontable,
  but already allocated memory is freed before the structures is filled
  with their default values.
  \param ft pointer to the csm_2d_functiontable structure
*/
static void reinit_2d_functiontable(csm_2d_functiontable *ft)
  { reinit_coordinates(&(ft->x));
    reinit_coordinates(&(ft->y));
    if (ft->z!=NULL)
      free(ft->z);
    ft->z= NULL;
    ft->value_cached= CSM_FALSE;
  }

        /*       lookup a value in the xy-table       */

/*! \internal \brief lookup a value in a csm_2d_functiontable structure [static]

  This function looks up the function value z for a given x and y in a
  csm_2d_functiontable structure which is a two-dimensional function
  table. The four points (x1,y1), (x1,y2), (x2,y1) and (x2,y2) that are
  closest to (x,y) are searched. A two-dimensional linear interpolation
  between four corresponding z values is done and this value is returned.
  \param ft pointer to the csm_2d_functiontable structure
  \param x x coordinate of the point to look up
  \param y y coordinate of the point to look up
  \return the value that fits best to (x,y) is returned.
*/
static double lookup_2d_functiontable(csm_2d_functiontable *ft,
				      double x, double y)
  { int res,xia,xib,yia,yib;
    double xa,xb,ya,yb;
    double zaa,zab,zba,zbb;
    double *z= ft->z;
    int no_of_columns;

    csm_coordinate cxa,cxb,cya,cyb;
    csm_coordinates *xcoords= &(ft->x);
    csm_coordinates *ycoords= &(ft->y);

    if (ft->value_cached)
      {
        if ((ft->x_last==x) && (ft->y_last==y))
          return(ft->z_last);
      }

    no_of_columns= ycoords->no_of_elements;

    res= coordinate_lookup_index(xcoords, x, &xia, &xib);
    if (res==-1) /* error */
      {
        ft->value_cached= CSM_FALSE;
        return(NAN);
      }

    res= coordinate_lookup_index(ycoords, y, &yia, &yib);
    if (res==-1) /* error */
      {
        ft->value_cached= CSM_FALSE;
        return(NAN);
      }

    /* res==2 with exact match is currently not treated separately */

    cxa= (xcoords->coordinate)[xia];
    cxb= (xcoords->coordinate)[xib];
    cya= (ycoords->coordinate)[yia];
    cyb= (ycoords->coordinate)[yib];

    xa= cxa.value; /* with exact match, xa==xb or ya==yb is possible */
    xb= cxb.value;
    ya= cya.value;
    yb= cyb.value;

    zaa= z[ cxa.index * no_of_columns + cya.index ];
    zab= z[ cxa.index * no_of_columns + cyb.index ];
    zba= z[ cxb.index * no_of_columns + cya.index ];
    zbb= z[ cxb.index * no_of_columns + cyb.index ];

#if 0
printf("x:%f y:%f xa:%f xb:%f ya:%f yb:%f\n",x,y,xa,xb,ya,yb);
printf("zaa:%f zab:%f zba:%f zbb:%f\n",zaa,zab,zba,zbb);
#endif
#if 1
    { double alpha = (xb==xa) ? 0 : (x-xa)/(xb-xa);
      double beta  = (yb==ya) ? 0 : (y-ya)/(yb-ya);
      if (alpha==0)               /* xb==xa ? */
        { 
          ft->value_cached= CSM_TRUE;
          ft->x_last= x;
          ft->y_last= y;
          if (beta==0)            /* yb==ya ? */
	    return(ft->z_last= zaa);
	  return(ft->z_last= zaa+ beta *(zab-zaa) );
	};
      if (beta==0)                /* yb==ya ? */
        {
          ft->value_cached= CSM_TRUE;
          ft->x_last= x;
          ft->y_last= y;
          return(ft->z_last= zaa+ alpha*(zba-zaa) );
        }

#if 0
printf(" ret: %f\n",
       zaa + beta*(zab-zaa)+alpha*(zba-zaa+beta*(zbb-zba-zab+zaa)));
#endif
      ft->value_cached= CSM_TRUE;
      ft->x_last= x;
      ft->y_last= y;
      return(ft->z_last= 
              zaa + beta*(zab-zaa)+alpha*(zba-zaa+beta*(zbb-zba-zab+zaa)) );
    };
#endif

#if 0
    { /* alternative according to formula given by J.Bahrdt */
      /* doesn't work when yb==ya or xb==xa */
      double delta= (yb-ya)*(xb-xa);
      double yb_  = yb - y;
      double xb_  = xb - x;
      double y_a  = y  - ya;
      double x_a  = x  - xa;
      return((zaa*yb_*xb_ + zab*y_a*xb_ + zba*yb_*x_a + zbb*y_a*x_a)/delta);
#endif

 }

    /*----------------------------------------------------*/
    /*@IL           generic functions (public)            */
    /*----------------------------------------------------*/

/*! \brief compute x from a given y

  This function computes x when y is given. Note that is doesn't
  work for two-dimensional function tables. For these, the
  function returns 0
  \param func pointer to the function object
  \param y the value of y
  \return x
*/

/*@EX(1)*/
double csm_x(csm_function *func, double y)
  { double ret;

    SEMTAKE(func->semaphore);

    if (func->on_hold)
      { ret= func->last_x;
        SEMGIVE(func->semaphore);
        return(ret);
      };

    switch (func->type)
      { case CSM_NOTHING:
               func->last_x= 0;
	       break;
	case CSM_LINEAR:
	       func->last_x= linear_get_x(&(func->f.lf), y);
	       break;
	case CSM_1D_TABLE:
	       func->last_x= lookup_1d_functiontable(&(func->f.tf_1),y,
	       					   CSM_TRUE);
	       break;
        default:
	       func->last_x=0;
	       break;
      };
    ret= func->last_x;
    SEMGIVE(func->semaphore);
    return(ret);
  }

/*! \brief compute y from a given x

  This function computes y when x is given. Note that is doesn't
  work for two-dimensional function tables. For these, the
  function returns 0
  \param func pointer to the function object
  \param x the value of x
  \return y
*/

/*@EX(1)*/
double csm_y(csm_function *func, double x)
  { double ret;

    SEMTAKE(func->semaphore);

    if (func->on_hold)
      { ret= func->last_y;
        SEMGIVE(func->semaphore);
        return(ret);
      };

    switch (func->type)
      { case CSM_NOTHING:
               func->last_y= 0;
	       break;
	case CSM_LINEAR:
	       func->last_y= linear_get_y(&(func->f.lf), x);
	       break;
	case CSM_1D_TABLE:
	       func->last_y= lookup_1d_functiontable(&(func->f.tf_1),x,
	       					   CSM_FALSE);
	       break;
        default:
	       func->last_y=0;
	       break;
      };
    ret= func->last_y;
    SEMGIVE(func->semaphore);
    return(ret);
  }

/*! \brief compute delta-x from a given delta-y

  This function computes a delta-x when a delta-y is given.
  Note that this function only works with linear functions.
  For all other types, this function
  returns 0.
  \param func pointer to the function object
  \param y the given delta-y
  \return delta-x
*/

/*@EX(1)*/
double csm_dx(csm_function *func, double y)
  { double ret;

    SEMTAKE(func->semaphore);

    if (func->on_hold)
      { ret= func->last_dx;
        SEMGIVE(func->semaphore);
        return(ret);
      };

    if (func->type==CSM_LINEAR)
      func->last_dx= linear_delta_get_x(&(func->f.lf), y);
    else
      func->last_dx=0;

    ret= func->last_dx;
    SEMGIVE(func->semaphore);
    return(ret);
  }

/*! \brief compute delta-y from a given delta-x

  This function computes a delta-y when a delta-x is given.
  Note that this function only works with linear functions.
  For all other types, this function
  returns 0.
  \param func pointer to the function object
  \param x the given delta-x
  \return delta-y
*/

/*@EX(1)*/
double csm_dy(csm_function *func, double x)
  { double ret;

    SEMTAKE(func->semaphore);

    if (func->on_hold)
      { ret= func->last_dy;
        SEMGIVE(func->semaphore);
        return(ret);
      };

    if (func->type==CSM_LINEAR)
      func->last_dy= linear_delta_get_y(&(func->f.lf), x);
    else
      func->last_dy=0;

    ret= func->last_dy;
    SEMGIVE(func->semaphore);
    return(ret);
  }

/*! \brief compute z from a given x and y

  This function computes z for two-dimensional function table
  when x and y are given. For all other function types, it returns
  0.
  \param func pointer to the function object
  \param x the value of x
  \param y the value of y
  \return z
*/

/*@EX(1)*/
double csm_z(csm_function *func, double x, double y)
  { double ret;

    SEMTAKE(func->semaphore);

    if (func->on_hold)
      { ret= func->last_z;
        SEMGIVE(func->semaphore);
        return(ret);
      };

    if (func->type==CSM_2D_TABLE)
      func->last_z= lookup_2d_functiontable(&(func->f.tf_2), x, y);
    else
      func->last_z=0;

    ret= func->last_z;
    SEMGIVE(func->semaphore);
    return(ret);
  }

    /*----------------------------------------------------*/
    /*@IL                initialization                   */
    /*----------------------------------------------------*/

      /*   initialize a csm_function object (private)   */

/*! \internal \brief re-init a csm_function structure [static]

  This function re-initializes a csm_function structure.
  Possibly allocated memory is freed before the structure
  is filled with its default values.
  \param func pointer to a csm_function structure
*/
static void reinit_function(csm_function *func)
  { if (func->type==CSM_NOTHING)
      { return; };
    if (func->type==CSM_1D_TABLE)
      reinit_1d_functiontable( &(func->f.tf_1) );
    if (func->type==CSM_2D_TABLE)
      reinit_2d_functiontable( &(func->f.tf_2) );

    /* @@@@ CAUTION: currently there is no locking,
       no test wether another process is currently
       accessing the structure */
    func->type= CSM_NOTHING;
  }

      /*  scan a string for a list of doubles (private) */

/*! \internal \brief tests wether a string is empty or a comment [static]

  This function returns CSM_TRUE when a string is empty or consists
  only of spaces or is a comment.
  \param st pointer to the string
  \return CSM_TRUE when the string is empty or a comment, CSM_FALSE
          otherwise
*/
static csm_bool str_empty_or_comment(char *st)
  { for(;*st!=0; st++)
      { if (isspace(*st))
          continue;
        if (*st=='#')
          return(CSM_TRUE);
	return(CSM_FALSE);
      };
    return(CSM_TRUE);
  }

      /*  scan a string for a list of doubles (private) */

/*! \internal \brief scans a string of space separated floating point numbers [static]

  This expects a string that contains floating point numbers separated
  by spaces. It reads these numbers and stores them consecutively in
  the array of doubles.
  \param st the string
  \param d a pointer to an array of doubles
  \param no_of_cols the expected number floating point numbers
  \return the number of floating point numbers actually read. The
          function returns 0 if there was an error, this means
          when the string contains parts that are neither numbers
          nor spaces.
*/
static int strdoublescan(char *st, double *d, int no_of_cols)
  /* returns the number of numbers found in the line, d may be null,
     in this case, the numbers are only counted,
     if a line contains not enough numbers, (unequal no_of_cols) the missing
     ones are set to 0 */
  /* reads over leading whitespaces */
  /* uses strtok ! */
  { int i;
    char *p= st;      /* anything different from NULL */
    char buf[1024];
    char *str= buf;
    double *dptr;

    if ((no_of_cols<=0)||(no_of_cols>1000))
      return(0);
    for( ;NULL!=strchr( " \t\r\n", *st); st++);
    if (*st==0)
      return(0);
    strncpy(buf, st, 1023); buf[1023]=0;
    if (d!=NULL)
      for(dptr= d, i=0; i<no_of_cols; i++, *(dptr++)=0 );
    for(p= str, dptr= d, i=0; i<no_of_cols; i++, dptr++)
      {
        if (NULL== (p= strtok(str," \t\r\n")))
	  return(i);
        str= NULL;
	if (d!=NULL)
          { if (1!=sscanf(p,"%lf",dptr))
	      return(0); /* a kind of fatal error */
          };
      };
    return(no_of_cols);  /* everything that was expected was found */
  }

      /*          clear a function (public)             */

/*! \brief clear a function

  This function clears the \ref csm_function structure. All
  allocated memory is released, the internal type is set to CSM_NOTHING.
  \param func pointer to the function object
*/

/*@EX(1)*/
void csm_clear(csm_function *func)
  { SEMTAKE(func->semaphore);
    func->on_hold= CSM_TRUE;
    SEMGIVE(func->semaphore);

    reinit_function(func);

    func->on_hold= CSM_FALSE;
  }

      /*   free all data for a function (public)        */

/*! \brief free a function-structure

  This function frees the \ref csm_function structure. All
  allocated memory is released, even the memory of the basic internal
  structure. This means that the cmd_function structure becomes
  invalid and cannot be used again later. Use this function with caution.
  \param func pointer to the function object
*/
/*@EX(1)*/
void csm_free(csm_function *func)
  { csm_clear(func);
    SEMDELETE(func->semaphore);
    free(func);
  }

      /*          define a function (public)            */

/*! \brief define a linear function

  This function initializes a \ref csm_function structure as
  a linear function (y= a+b*x).
  \param func pointer to the function object
  \param a offset of y= a+b*x
  \param b multiplier of y= a+b*x
*/

/*@EX(1)*/
void csm_def_linear(csm_function *func, double a, double b)
  /* y= a+b*x */
  { SEMTAKE(func->semaphore);
    func->on_hold= CSM_TRUE;
    SEMGIVE(func->semaphore);

    reinit_function(func);

    func->type= CSM_LINEAR;
    (func->f.lf).a= a;
    (func->f.lf).b= b;

    func->on_hold= CSM_FALSE;
  }

/*! \brief re-define the offset of a linear function

  This function re-defines the offset-factor of
  a linear function (y= a+b*x).
  \param func pointer to the function object
  \param a offset of y= a+b*x
  \return returns CSM_FALSE in case of an error, CSM_TRUE otherwise
*/

/*@EX(1)*/
csm_bool csm_def_linear_offset(csm_function *func, double a)
  { if (func->type!=CSM_LINEAR)
      { DBG_MSG_PRINTF2("error in csm_def_linear_offset line %d,\n" \
                        "not a linear function!\n", __LINE__);
        return(CSM_FALSE);
      };

    SEMTAKE(func->semaphore);
    func->f.lf.a= a;
    SEMGIVE(func->semaphore);

    return(CSM_TRUE);
  }

/*! \brief read parameters of one-dimensional function table

  This function reads a one-dimensional function-table from a
  file.
  \param filename the name of the file
  \param func pointer to the function object
  \return returns CSM_FALSE in case of an error, CSM_TRUE otherwise
*/

/*@EX(1)*/
csm_bool csm_read_1d_table(char *filename, csm_function *func)
/* if len==0, the table length is determined by counting the number of lines
   in the file from the current position to it's end */
  { char line[128];
    long pos;
    long i;
    long errcount;
    csm_1d_functiontable *ft= &(func->f.tf_1);
    csm_coordinate *xc, *yc;
    int len;
    FILE *f;
    char dummy;

    if (NULL==(f=fopen(filename,"r"))) /* vxworks doesn't accept "rt" */
      { DBG_MSG_PRINTF2("error in csm_read_xytable line %d,\n" \
                        "file open error!\n", __LINE__);
        return(CSM_FALSE);
      };

    pos= ftell(f);
    for(len=0; NULL!=fgets(line, 127, f); len++);
    if (-1==fseek(f, pos, SEEK_SET))
      { fclose(f);
        return(CSM_FALSE);
      };

    SEMTAKE(func->semaphore);
    func->on_hold= CSM_TRUE;
    SEMGIVE(func->semaphore);

    reinit_function(func);

    init_1d_functiontable(ft);

    if (!alloc_coordinates(&(ft->x), len))
      { fclose(f);
	reinit_function(func);
        func->on_hold= CSM_FALSE;
	return(CSM_FALSE);
      };

    if (!alloc_coordinates(&(ft->y), len))
      { fclose(f);
	reinit_function(func);
        func->on_hold= CSM_FALSE;
	return(CSM_FALSE);
      };

    xc= (ft->x).coordinate;
    yc= (ft->y).coordinate;

    for(errcount=0, i=0;(len>0) && (NULL!=fgets(line, 127, f)); len--)
      { if (str_empty_or_comment(line))
          continue;
        if (2!=sscanf(line, " %lf %lf %c",
                      &(xc->value), &(yc->value), &dummy))
          { DBG_MSG_PRINTF5("warning[%s:%d]: the following line of the "
	           "data-file (%s) was not understood:\n%s\n",
		   __FILE__,__LINE__,filename,line);
	    if (++errcount<4)
	      continue;
	    fclose(f);
	    reinit_function(func);
            func->on_hold= CSM_FALSE;
            DBG_MSG_PRINTF3("error[%s:%d]: too many errors in file\n",
			  __FILE__,__LINE__);
            return(CSM_FALSE);
          };
	xc->index= i;
	yc->index= i;
        i++, xc++,yc++;
      };
    if (i<=0)
      { fclose(f);
	reinit_function(func);
        func->on_hold= CSM_FALSE;
        DBG_MSG_PRINTF3("error[%s:%d]: no data was found at all\n",
		      __FILE__,__LINE__);
        return(CSM_FALSE);
      };

    if (!resize_coordinates(&(ft->x), i))
      { fclose(f);
	reinit_function(func);
        func->on_hold= CSM_FALSE;
        return(CSM_FALSE);
      };
    if (!resize_coordinates(&(ft->y), i))
      { fclose(f);
	reinit_function(func);
        func->on_hold= CSM_FALSE;
        return(CSM_FALSE);
      };
    fclose(f);

    func->type= CSM_1D_TABLE;

    coordinate_sort(&(ft->x));
    coordinate_update_backlinks(&(ft->x), &(ft->y));
    coordinate_sort(&(ft->y));
    coordinate_update_backlinks(&(ft->y), &(ft->x));
    func->on_hold= CSM_FALSE;
    return(CSM_TRUE);
  }

/*! \brief read parameters of two-dimensional function table

  This function reads a two-dimensional function-table from a
  file.
  \param filename the name of the file
  \param func pointer to the function object
  \return returns CSM_FALSE in case of an error, CSM_TRUE otherwise
*/

/*@EX(1)*/
csm_bool csm_read_2d_table(char *filename, csm_function *func)
/* file format:
    y1   y2   y3  ..
x1  z11  z12  z13 ...
x2  z21  z22  z23 ...
.    .    .    .
.    .    .    .
.    .    .    .
*/
  { char line[1024];
    long pos;
    long i,j,lines,errcount;
    double *buffer;
    double *zptr;
    csm_2d_functiontable *ft= &(func->f.tf_2);
    csm_coordinate *xc, *yc;
    FILE *f;
    int columns,rows;

    if (NULL==(f=fopen(filename,"r"))) /* vxworks doesn't accept "rt" */
      { DBG_MSG_PRINTF2("error in csm_read_xytable line %d,\n" \
                        "file open error!\n", __LINE__);
        return(CSM_FALSE);
      };

    if (NULL==fgets(line, 1024, f))
      { fclose(f);
        return(CSM_FALSE);
      };

    columns= strdoublescan(line, NULL, 512);
    if (columns<1)
      { fclose(f);
        return(CSM_FALSE);
      };
    if (NULL==(buffer= malloc(sizeof(double)*(columns+1))))
      { DBG_MSG_PRINTF2("MALLOC FAILED in file %s\n",__FILE__);
        fclose(f);
	return(CSM_FALSE);
      };
    if (columns!= (i= strdoublescan(line, buffer, columns)))
      { DBG_MSG_PRINTF3("unexpected err in line %d in file %s\n",
                        __LINE__,__FILE__);
        free(buffer);
	fclose(f);
	return(CSM_FALSE);
      };

    SEMTAKE(func->semaphore);
    func->on_hold= CSM_TRUE;
    SEMGIVE(func->semaphore);

    reinit_function(func);
    init_2d_functiontable(ft);

    if (!alloc_coordinates(&(ft->y), columns))
      { free(buffer);
        reinit_function(func);
	fclose(f);
        func->on_hold= CSM_FALSE;
	return(CSM_FALSE);
      };

    yc= (ft->y).coordinate;

    /* copy y-values into the csm_table_function - structure */
    { double *r= buffer;
      csm_coordinate *c= yc;
      for(i=0; i< columns; i++, c++, r++ )
        { c->value= *r;
	  c->index= i;
	};
    };

    pos= ftell(f);
    for(lines=0; NULL!=fgets(line, 1024, f); lines++);
    if (-1==fseek(f, pos, SEEK_SET))
      { free(buffer);
        reinit_function(func);
	fclose(f);
        func->on_hold= CSM_FALSE;
	return(CSM_FALSE);
      };
    rows= lines;

    if (!alloc_coordinates(&(ft->x), rows))
      { free(buffer);
        reinit_function(func);
	fclose(f);
        func->on_hold= CSM_FALSE;
	return(CSM_FALSE);
      };

    xc= (ft->x).coordinate;

    if (!init_matrix(&(ft->z), rows, columns))
      { free(buffer);
        reinit_function(func);
	fclose(f);
        func->on_hold= CSM_FALSE;
	return(CSM_FALSE);
      };

    zptr= ft->z;

    for(errcount=0, i=0; (lines>0) && (NULL!=fgets(line, 1024, f)); lines--)
      {
        if (str_empty_or_comment(line))
          continue;
        if (columns+1 != strdoublescan(line, buffer, columns+1))
          { DBG_MSG_PRINTF5("warning[%s:%d]: the following line of the "
	                    "data-file (%s) was not understood:\n%s\n",
			    __FILE__,__LINE__,filename,line);
	    if (++errcount<4)
	      continue;
	    fclose(f);
	    reinit_function(func);
            func->on_hold= CSM_FALSE;
            DBG_MSG_PRINTF3("error[%s:%d]: too many errors in file\n",
			  __FILE__,__LINE__);
            return(CSM_FALSE);
	  };

        xc->value= buffer[0];
	xc->index= i;

	zptr= &((ft->z)[i*columns]);
	for (j=0; j<columns; j++)
	  zptr[j]= buffer[j+1];

	i++, xc++;
      };

    if (i<=0)
      { free(buffer);
      	reinit_function(func);
        fclose(f);
        func->on_hold= CSM_FALSE;
        DBG_MSG_PRINTF3("error[%s:%d]: no data was found at all\n",
		      __FILE__,__LINE__);
        return(CSM_FALSE);
      };

    rows= i;
    if (!resize_coordinates(&(ft->x), rows))
      { free(buffer);
        reinit_function(func);
	fclose(f);
        func->on_hold= CSM_FALSE;
	return(CSM_FALSE);
      };

    fclose(f);
    free(buffer);

    func->type= CSM_2D_TABLE;

    coordinate_sort(&(ft->x));
    coordinate_sort(&(ft->y));
    func->on_hold= CSM_FALSE;
    return(CSM_TRUE);
  }


      /*         initialize the module (public)         */

/*! \brief initialize the module

  This function should be called once to initialize the
  module.
*/

/*@EX(1)*/
void csm_init(void)
  {
    if (initialized)
      return;
    initialized= CSM_TRUE;
#if USE_DBG
    csm_dbg_init();
#endif
  }

      /*          create a new function object          */


/*! \brief create a new function object

  This function returns a pointer to a new function object. The object
  is initialized as an empty function
  \return returns NULL in case of an error. Otherwise the
          pointer to the csm_function is returned
*/

/*@EX(1)*/
csm_function *csm_new_function(void)
  {
    csm_function *f= malloc(sizeof(csm_function));

    if (f==NULL)
      return(NULL);

    f->type= CSM_NOTHING;
    f->last_x = 0;
    f->last_y = 0;
    f->last_dx= 0;
    f->last_dy= 0;
    f->last_z = 0;
    f->on_hold= CSM_FALSE;
    if (SEMCREATE(f->semaphore))
      { free(f);
        return(NULL);
      };
    return((csm_function *)f);
  }

    /*----------------------------------------------------*/
    /*@IL              debugging (public)                 */
    /*----------------------------------------------------*/

/*! \internal
    \brief print a csm_coordinates structure [static]

  This prints a csm_coordinates structure to the screen.
  \param c pointer to the csm_coordinates structure
*/
static void csm_pr_coordinates(csm_coordinates *c)
  { int i;
    printf("no . of elements: %d\n", c->no_of_elements);
    if (c->a_last==-1)
      printf("(still in initial-state)\n");
    else
      { printf("a_last:%d  b_last:%d\n",
        c->a_last, c->b_last);
      };
    for(i=0; i< c->no_of_elements; i++)
      printf(" [%03d]: %12f --> %3d\n",
             i,(c->coordinate)[i].value,(c->coordinate)[i].index);
  }

/*! \internal
    \brief print a csm_1d_functiontable structure [static]

  This print an element of a csm_1d_functiontable structure to the screen.
  \param t pointer to the csm_1d_functiontable
  \param index index of the element
*/
static void csm_pr_1d_table_elm(csm_1d_functiontable *t, int index)
  { printf("x: %15f   y: %15f\n",
           ((t->x).coordinate)[index].value,
           ((t->y).coordinate)[index].value);
  }

/*! \internal
    \brief print a csm_linear_function structure [static]

  This prints a csm_linear_function structure to the screen.
  \param lf pointer to the csm_linear_function
*/
static void csm_pr_linear(csm_linear_function *lf)
  { printf("a:%f  b:%f  (y=a+b*x)\n", lf->a, lf->b); }

/*! \internal
    \brief print a csm_1d_functiontable structure [static]

  This prints a csm_1d_functiontable structure to the screen.
  \param tf pointer to the csm_1d_functiontable
*/
static void csm_pr_1d_table(csm_1d_functiontable *tf)
  { int i;
    int l= (tf->x).no_of_elements;

    if (tf->x.a_last==-1)
      printf("x-coord is still in initial-state\n");
    else
      printf("x-a_last:%d  x-b_last:%d\n", tf->x.a_last, tf->x.b_last);
    if (tf->y.a_last==-1)
      printf("x-coord is still in initial-state\n");
    else
      printf("y-a_last:%d  y-b_last:%d\n", tf->y.a_last, tf->y.b_last);
    printf("number of elements: %d\n", l);
    for(i=0; i<l; i++)
      csm_pr_1d_table_elm( tf, i);
  }

/*! \internal
    \brief print a csm_2d_functiontable structure [static]

  This prints a csm_2d_functiontable structure to the screen.
  \param ft pointer to the csm_2d_functiontable
*/
static void csm_pr_2d_table(csm_2d_functiontable *ft)
  { int i,j;
    int rows   = ft->x.no_of_elements;
    int columns= ft->y.no_of_elements;
    double *z= ft->z;
    csm_coordinate *xc= ft->x.coordinate;
    csm_coordinate *yc= ft->y.coordinate;

    printf("rows:%d  columns:%d\n",
           rows, columns);
    printf("%18s"," ");
    for (j=0; j<columns; j++)
      printf("%15f | ", yc[j].value);
    printf("\n");
    printf("------------------");
    for (j=0; j<columns; j++)
      { printf("---------------"); };
    for(i=0; i<rows; i++)
      { printf("%15f | ", xc[i].value);
        for (j=0; j<columns; j++)
          printf("%15f | ", z[columns*i+j]);
	printf("\n");
      };
  }

/*! \brief dump a new function object

  This printfs information about the given function to
  stdout.
  \param func pointer to the function object
*/
/*@EX(1)*/
void csm_pr_func(csm_function *func)
  { if (func->on_hold)
      printf("function is on hold!\n");
    else
      printf("function is operational\n");

    printf("Last calculated values: x:%f y:%f z:%f dx:%f dy%f\n",
           func->last_x, func->last_y, func->last_z,
           func->last_dx, func->last_dy);

    printf("function type: \n");
    switch( func->type)
      { case CSM_NOTHING:
             printf("CSM_NOTHING\n");
             break;
        case CSM_LINEAR:
             printf("CSM_LINEAR\n");
             csm_pr_linear(&(func->f.lf));
             break;
        case CSM_1D_TABLE:
             printf("CSM_1D TABLE\n");
             printf("normal function:\n");
             csm_pr_1d_table(&(func->f.tf_1));
             break;
	case CSM_2D_TABLE:
             printf("CSM_2D_TABLE\n");
	     csm_pr_2d_table(&(func->f.tf_2));
             break;
        default:
             printf("%d (unknown)\n", func->type);
             break;
      };
  }


/*@EM("#endif\n") */

#if 0
csm_free implementieren.

strukturen cachen (anhand des Filenamens)

Filenamen �bergeben

evtl die csm-struktur direkt allozieren

reload von Files implementieren
  im Fehlerfall die alten Daten behalten


#endif
