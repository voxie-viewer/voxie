## Description

The Small Pore Removal filter can be used to remove unwanted pores specified by a minimum volume size. Each pore smaller than the minimum volume size will be removed.

The filter can be used in two diffrent modes: The Pore Space and the Solid Space mode.

Both modes are implemented with the Module Morphology from scikit-image. [Morphology on scikit](https://scikit-image.org/docs/dev/api/skimage.morphology.html) 


### Pore Space

Remove objects smaller than the specified minimum volume size. 

The following method from the class smallPoreRemoval.py implements the Pore Space.

```outputBuffer.array[:] = morphology.remove_small_objects(image, min_size=area, connectivity=1, in_place=False)```

[Remove small objects on scikit](https://scikit-image.org/docs/dev/api/skimage.morphology.html#skimage.morphology.remove_small_objects)
    
    

### Solid Space

Remove contiguous holes smaller than the specified minimum volume size. 

The following method from the class smallPoreRemoval.py implements the Solid Space.

```outputBuffer.array[:] = morphology.remove_small_holes(image, min_size=area, connectivity=1, in_place=False)```

[Remove small holes on scikit](https://scikit-image.org/docs/dev/api/skimage.morphology.html#skimage.morphology.remove_small_holes)








