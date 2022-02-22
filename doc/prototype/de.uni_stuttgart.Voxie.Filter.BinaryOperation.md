## Description

The Binary Operation filter was implemented with **numpy** and allows the user to apply logical-operations to binary volumes.

[Logical-operations on numpy](https://docs.scipy.org/doc/numpy-1.13.0/reference/routines.logic.html#logical-operations)


## Properties

The filter got four diffrent operators: NOT, AND, OR, XOR.

### NOT
The logical-operator NOT requires a single volume v1 as input and computes the truth value of NOT v1 element-wise.

Example

```np.logical_not([True, False, 0, 1])```

```array([False,  True,  True, False])```

[Logical_NOT on numpy](https://docs.scipy.org/doc/numpy/reference/generated/numpy.logical_not.html#numpy.logical_not) 


### AND
The logical-operator AND requires two volumes v1 and v2 and computes the truth value of v1 AND v2 element-wise.

Example

```np.logical_and([True, False], [False, False])```

```array([False, False])```

[Logical_AND on numpy](https://docs.scipy.org/doc/numpy/reference/generated/numpy.logical_and.html#numpy.logical_and)


### OR
The logical-operator OR requires two volumes v1 and v2 and computes the truth value of v1 OR v2 element-wise.

Example

```np.logical_or([True, False], [False, False])```

```array([ True, False])```

[Logical_OR on numpy](https://docs.scipy.org/doc/numpy/reference/generated/numpy.logical_or.html#numpy.logical_or)


### XOR
The logical-operator XOR requires two volumes v1 and v2 and computes the truth value of v1 XOR v2 element-wise.

Example

```np.logical_xor([True, True, False, False], [True, False, True, False])```

```array([False,  True,  True, False])``` 


[Logical_XOR on numpy](https://docs.scipy.org/doc/numpy/reference/generated/numpy.logical_xor.html#numpy.logical_xor)
