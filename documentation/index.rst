EPICS csm Module
================

Authors:
--------

   `Götz Pfeiffer`_ (`HZB`_)
      wrote the csm library

   `Ben Franksen`_ (`HZB`_)
      wrote the cvtRecord

About
-----

This module contains two parts: The first is a generic (EPICS-independent)
library implementing one- and two-dimensional interpolation tables, including
procedures to load and store them from a file system. The second is an EPICS
record type specifically designed to make it easy to do conversion via
interpolation table and other means. The cvtRecord uses the csm library to
achieve conversion via tables.


Download
--------

You can download a release from the links in the table below, or get the
latest development version via `darcs`_::

   darcs get http://www-csr.bessy.de/control/SoftDist/csm/repo/csm

You can also `browse the repository`_.

 ========= =============== ============================ ===============
  Version   EPICS Release   Filename                     Release Notes
 ========= =============== ============================ ===============
    4.2       3.14.12.2     :download:`csm-4.2.tar.gz`    :ref:`R4-2`
 --------- --------------- ---------------------------- ---------------
    4.1       3.14.12.2     :download:`csm-4.1.tar.gz`    :ref:`R4-1`
 --------- --------------- ---------------------------- ---------------
    4.0       3.14.12.2     :download:`csm-4.0.tar.gz`    :ref:`R4-0`
 --------- --------------- ---------------------------- ---------------
    3.7       3.14.12.2     :download:`csm-3.7.tar.gz`    :ref:`R3-7`
 --------- --------------- ---------------------------- ---------------
    3.6       3.14.8.2      :download:`csm-3.6.tar.gz`    :ref:`R3-6`
 --------- --------------- ---------------------------- ---------------
    3.5       3.14.8.2      :download:`csm-3.5.tar.gz`    :ref:`R3-5`
 --------- --------------- ---------------------------- ---------------
    3.4       3.14.8.2      :download:`csm-3.4.tar.gz`    :ref:`R3-4`
 --------- --------------- ---------------------------- ---------------
    3.3       3.14.8.2      :download:`csm-3.3.tar.gz`    :ref:`R3-3`
 --------- --------------- ---------------------------- ---------------
    3.2       3.14.8.2      :download:`csm-3.2.tar.gz`        n/a
 ========= =============== ============================ ===============


Documentation
-------------

For the csm library, have a look at the `Doxygen generated API docs`_,
otherwise see :doc:`cvtRecord`.

File Format
^^^^^^^^^^^

There are two kinds of table files accepted by the csm library:
one-dimensional and two-dimensional. For both formats the file consists of a
number of lines; each line must not be longer than 1023 bytes (including the
line terminator(s)) for two-dimensional tables and 127 bytes for
one-dimensional tables.

Lines that are empty (i.e. consist only of white space), or start with a '#'
byte (possibly preceded by white space) are silently ignored.

All other lines should consist of two or more *elements*, separated by (any
positive amount of) white space (including tabs). Leading and trailing white
space is ignored. The number of elements per line must not be greater than
512 for two-dimensional tables, and 2 for on-dimensional tables.

Elements are whatever scanf accepts when given the "%lf" format specifier,
i.e. standard C floating point literals.

A one-dimensional table specifies a function of with one parameter. It must
have exactly two elements per line. The first element is the X coordinate,
the second the Y coordinate. The csm library has functions to convert in
both directions (see `csm_x`_ and `csm_y`_).

A two-dimensional table specifies a function with two parameters. The first
line and the first column specify the XY-grid. The first line must have
exactly one element less than the remaining lines; it specifies the Y
coordinates of the grid. The first column (i.e. the first elements of the
remaining lines) specify the X coordinates of the grid, while the remaining
elements specify the value (Z coordinate) at the corresponding point in the
grid (see `csm_z`_).

Lines and columns can be specified in any order. Particularly, there is no
need to specify them in ascending or descending order. However, for
one-dimensional tables, the result is only well-defined if the table
actually defines a function in the specified direction. That is, equal input
coordinates should map to equal output coordinates. Also, for non-monotonic
functions, `csm_x`_ is not the inverse of `csm_y`_.

Problems
--------

If you have any problems with this module, send a mail to one of the
authors.


.. _Ben Franksen: mailto:benjamin.franksen@helmholtz-berlin.de
.. _Götz Pfeiffer: mailto:goetz.pfeiffer@helmholtz-berlin.de
.. _darcs: http://www.darcs.net/
.. _csm-3.2.tar.gz: csm-3.2.tar.gz
.. _HZB: http://www.helmholtz-berlin.de/
.. _EPICS: http://www.aps.anl.goc/epics/
.. _browse the repository: http://www-csr.bessy.de/cgi-bin/darcsweb.cgi?r=csm;a=summary
.. _Doxygen generated API docs: csmApp/html/csmbase_8c.html
.. _csm_x: csmApp/html/csmbase_8c.html#6226f2df9d594321101657cd5c53bb7d
.. _csm_y: csmApp/html/csmbase_8c.html#c28ee80fa3bcc8174ff0844ff92e981f
.. _csm_z: csmApp/html/csmbase_8c.html#c0e3dcd535ce486f004128f9c270cb2b
