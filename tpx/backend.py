# This script is meant to allow toggling between a variety of backends:
# - numpy, jax.numpy, cupy, etc.
# The goal is to infer what is available and pick them in some order.

try:
    import numpy as numpy
except:
    pass