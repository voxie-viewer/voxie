## Description

Different Operations and Shapes can be used on a volume dataset.
To see how exactly the Morphological Operations filter is calculated, show the source code.

## Parameters

The following list details which parameters are used by the filter.

### Operation

- Opening
- Closing
- Erosion
- Dilation

Opening is the selected Default Mode.

### Shape

- Diamond
- Sphere
- Cube

The Diamond shape uses the Manhattan distance calculation.

```distance = abs(x - center) + abs(y - center) + abs(z - center)```

Sphere shape uses the Euclid distance calculation.

```distance = math.sqrt((x - center) ** 2 + (y - center) ** 2 + (z - center) ** 2)```

Cube shape uses the Chessboard distance.

```distance = max([abs(x - center), abs(y - center), abs(z - center)])```

Diamond is the selected Default Mode.

### Size

Size value, Default Value 1, Minimum Value 1

### Dataset is segmented

Is the Dataset segmented or not? Default is it's segmented (checked)
