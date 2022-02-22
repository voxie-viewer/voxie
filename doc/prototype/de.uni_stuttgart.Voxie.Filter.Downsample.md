## Description

As the name indicates, the purpose of the Downsample-filter is the downsampling of a volume data set. The resulting data set can be saved and reused afterwards. The factor of the downsampling must be set as a integer value by the user.

### Calculation

Each dimension of the dataset will be divided by the set factor.

An example: 
A volume dataset with the dimensions [495x451x495] will result into [247x225x247] after a downsampling with the factor 2.


### Saving the downsampled volume data set

To save the downsampled dataset select the object 'Output of Downsample Volume' and use the save button, shown as diskette, over the Filename.


