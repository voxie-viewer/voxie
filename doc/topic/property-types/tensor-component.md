Tensor component property type
==============================

The property type `de.uni_stuttgart.Voxie.PropertyType.TensorComponent`
specifies a compoment of a tensor.

DBus signature
--------------

The DBus signature of the type is `(auau)`.

The data consist of two lists:

- a list of contravariant indices
- a list of covariant indicies

All indices are 0-based.

Examples
--------

- The value `[[2], []]` would represent dimension 2 (0-based) / the Z-dimension of an euclidean vector.
- The value `[[], [0]]` would represent the first value of a covector.
- The value `[[1], [2]]` would represent the value in row 1, column 2 (both 0-based) of a matrix which is used to specify a linear transformation.

See also <https://en.wikipedia.org/w/index.php?title=Tensor&oldid=1286583045#Examples>.
