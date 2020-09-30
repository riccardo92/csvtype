Installing
----------

PyPI
^^^^

Requirements: MacOS >= 10.13 or (basically) any Linux distro (i686 or x86-64).
3.6 <= Python <= 3.8

Run

::

    pip install csvtype

From source
^^^^^^^^^^^

Requirements: MacOS or (basically) any Linux distro and boost dynamic libraries and headers (boost and boost-devel) and of course a >= C++11 compatible clang/g++ compiler.


Clone the repository and run

::

    python setup.py build
    pip install -e .
