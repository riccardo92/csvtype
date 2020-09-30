# csvtype

csvtype is a Python package with pybind11 bindings to C++ code that can
infer what column types a CSV consists of. It can do so by processing the file
line by line so that memory usage is very limited while still being fast enough.

The column types can be defined using PCRE regex patterns.

You can find the documentation at: 

## Installation

## From PyPI

Requirements: MacOS >= 10.13 or (basically) any Linux distro (i686 or x86-64).
3.6 <= Python <= 3.8

Run

```bash
pip install csvtype
```

## Building from source

Requirements: MacOS or (basically) any Linux distro and boost dynamic libraries and headers (boost and boost-devel) and of course a >= C++11 compatible clang/g++ compiler.


 - Clone this repository
 - Run:`

  ```bash
  python setup.py build
  pip install -e .
  ```

### Windows
TODO

## Developing

Create a virtual environment first.

```
python3 -m venv venv
source venv/bin/activate
```

install necessary packages

```
pip install -r requirements.txt
```

### Building the documentation

Documentation is auto generated from NumPy style doc strings in `csvtype/inferencer.py`.

To build:

 - `cd csvtype/docs`
 - `make html`

### Linting

Run `make lint` from the root folder.

### Unit tests

Run `make test` from the root folder.
