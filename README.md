# TPX

This repository contains code to read timepix files from binary and convert them into numpy arrays.

In time, it may contain tools for analyzing this data too.

## Installation

You need python, numpy, pybind11, and a C++ compiler.  Install the requirements first:

`pip install -r requirements.txt`

And then, install this package:

`python setup.py install --user`

To remove it, do `pip uninstall tpx`.

## Usage

Currently, there is one and only one function available.  Use it like this:

```python
>>> import tpx
>>> tpx_data = tpx.parse_binary_file("camera-processing/II-in-sub/frames_000002.tpx3")
>>> print(type(tpx_data))
<class 'numpy.ndarray'>
>>> print(tpx_data.dtype)
[('TOA', '<f8'), ('TOT', '<f8'), ('x', '<i4'), ('y', '<i4')]
```

As you can see, the data comes in a numpy structured array with 4 columns: Time of Arrival (TOA), Time Over Threshold (TOT), and x/y pixel.
