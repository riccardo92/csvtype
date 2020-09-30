csvtype
=======

csvtype is a Python package with pybind11 bindings to C++ code that can
infer what column types a CSV consists of. It can do so by processing the file
line by line so that memory usage is very limited while still being fast enough.

The column types can be defined using PCRE regex patterns.

Table of contents
-----------------
.. toctree::
   :maxdepth: 2

   installing.rst
   inferencer.rst
