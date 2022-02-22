`.timepixraw.json` files
========================

`.timepixraw.json` files contain metadata about a series of scans performed by one or more *Timepix3* detectors.

- `"Type"`: Always the string `"TimepixRawData"`
- `"Detectors"`: A JSON array of *Detector* JSON objects, describing the array of detectors used to record the event stream files comprising this dataset
- `"Streams"`: A JSON array of *Stream* JSON objects, containing the filenames of all scans comprising this dataset

All file names are given as relative paths to the location of the `.timepixraw.json` file.

*Detector* objects
------------------

Each *Detector* object represents a detector that was used to make a series of scans as part of this dataset.

The following keys are supported:

- `"ChipboardID"`: A string containing the ID of the readout chip used by this detector.
- `"Origin"`: An array with 2 values, describing the position of the top left corner of this detector relative to the whole array, in meters.
- `"Rotation"`: A number describing the 2D rotation angle of the detector in radians. The detector is assumed to be rotated around its top left corner, and the `Origin` must adjust for this accordingly.
- `"PixelSize"`: An array with 2 values, describing the width and height of each pixel in meters.
- `"CalibrationFile"`: A string containing the name of the calibration file for this detector in the XML file format.

During the *Clustering* step, the `Origin`, `Rotation` and `PixelSize` will be applied as a translation, rotation and scale to all events loaded from streams corresponding to this detector.

The `CalibrationFile` is required to convert raw *Time over Threshold* values from the `.t3r` file into energy levels in units of kilo-electron-Volts (keV) during the *Clustering* step.


*Stream* objects
----------------

Each *Stream* object describes a scan or group of scans made over a certain timespan by each detector in the array.

The following key is supported:

- `"Data"`: An array of strings, each storing the name of the `.t3r` file containing the data for this stream. The index of each filename in the array must match the index of the detector it was recorded by in the *Detectors* array, as the calibration settings, coordinate transformations and White Image of this detector will be associated with the stream. 
- `"WhiteImage"`: If this key is included, this stream is a *White Image* dataset, which is a scan performed without an object.

Semantically, scans within the same stream are assumed to have been recorded simultaneously and will be stitched together in later processing stages.

Other keys are also acceptable for use as arbitrary per-scan metadata (such as the rotation angle of the object being scanned).

The `WhiteImage` data is optional, and can be used to correct for artifacts and fluctuations originating from the detector itself during the *Projection* step. This converts the energy levels from an absolute value in *keV* to a unitless, normalized value ranging from 0 to 1, representing the fraction of total energy each pixel recorded during the scan of an object relative to the white reference scan.
