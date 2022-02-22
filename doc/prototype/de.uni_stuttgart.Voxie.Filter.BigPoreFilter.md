## Description

The Big Pore Removal Filter can be used to remove unwanted pores specified by a  volume size. Each pore over a set volume will be removed from the dataset.

The filter can be used in two diffrent modes: The Pore Space and the Solid Space mode.

Both modes are implemented with the Module Morphology from scikit-image. [Morphology on scikit](https://scikit-image.org/docs/dev/api/skimage.morphology.html)

This filter is the counterpart of the Small Pore Removal Filter, which removes pores under a set volume. For this reason the implementation is identical, besides an  negation before the calculation.

### Pore Space

Remove objects bigger than the specified minimum volume size.

The following method from the class smallPoreRemoval.py implements the Pore Space.

```outputBuffer.array[:] = morphology.remove_small_objects(image, min_size=area, connectivity=1, in_place=False)```

[Remove small objects on scikit](https://scikit-image.org/docs/dev/api/skimage.morphology.html#skimage.morphology.remove_small_objects)


### Solid Space

Remove contiguous holes bigger than the specified minimum volume size.

The following method from the class smallPoreRemoval.py implements the Solid Space.

```outputBuffer.array[:] = morphology.remove_small_holes(image, min_size=area, connectivity=1, in_place=False)```

[Remove small holes on scikit](https://scikit-image.org/docs/dev/api/skimage.morphology.html#skimage.morphology.remove_small_holes)








