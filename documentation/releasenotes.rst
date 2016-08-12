Release Notes
=============

.. _R4-3:

Changes since R4-2
------------------

* cvtRecord: added BDIR/NBDI fields

  The directory where a table file is located is now constructed by
  concatenating the new BDIR with the TDIR fields (with a "/" in between).
  This is a work-around for the 40 character string limitation in EPICS.
  Specifically, there is currently no existing record type that can
  calculate a string concatenation such that the result is longer than 40
  characters.

.. _R4-2:

Changes since R4-1
------------------

* fix in cvtRecord support: must not pass 0 to epicsMessageQueueCreate

* added documentation of file format to index page

.. _R4-1:

Changes since R4-0
------------------

Csmbase optimized for speed.

* The search for a matching interval in a table of break-points was optimized.
  The lookup function now looks first to the previous interval, then the
  intervals below and above the previous one. Only if these intervals don't
  match it performs a binary search across the while break point table.
* If the functions csm_x, csm_y or csm_z are called with the same values as the
  last time, they do not perform any calculation but return the previous value
  from their internal cache.

.. _R4-0:

.. _R3-7:

Changes since R3-6
------------------

Only internal changes.

.. _R3-6:

Changes since R3-5
------------------

* default base dependency changed to 3-14-12-1-1
* added dir dependencies for parallel make

.. _R3-5:

Changes since R3-4
------------------

* The csm library now handles one-dimensional tables with just one
  point correctly as a constant function. Previously they returned
  NAN, except if the input was exactly at the specified point.
* Empty one-dimensional tables (with no points defined) now return
  NAN instead of zero.

.. _R3-4:

Changes since R3-3
------------------

* cvtRecord: removed "invalid pv link" error messages.
  This is consistent with how the records in base work and avoids
  misleading error messages at startup time when CA input links
  have not yet connected.
* removed dir creation from upload target actions
* cvtRecord: layout changes, renamed csm_alarm to checkAlarms
* removed obsolete cvtRecord.html

.. _R3-3:

Changes since R3-2
------------------

* fixed performance problem by moving inactive mode related link
  parsing from process to init_record
* moved db_post_events call for X and Y fields to monitor routine
* replaced DBF_FLOAT with DBF_DOUBLE in cvt limit fields
* added sphinx generated documentation/download website
* added boringfile
* added hgen.pl to make module self-contained
