TomographyRawData Geometries
============================

Possible geometry types:
* `ConeBeamCT`
* `ConeBeamCTRepeat0`
* `MovementReferenceData`

ConeBeamCT
----------

`ConeBeamCT` indicates a normal cone beam CT geometry.

Example JSON info:

```
{
  "DistanceSourceAxis": 0.05504079437255859, // optional
  "DistanceSourceDetector": 1.2818969726562501, // optional
  "AngularStep": 0.0017453292519943296, // optional
  "InitialAngle": 0.0, // optional (same as Images[0]['TableRotAngle'])
  "NumberOfAngles": 3600, // optional (same as count(Images))
  "RotationDirection": -1, // optional
  // A (possibly incomplete) ProjectionGeometry, values can be overwritten by ProjectionGeometry elements deeper in the tree
  "ProjectionGeometry": { // optional
      "ProjectionOrigin": [
          -0.2032,
          -0.146304
      ],
      "ProjectionSize": [
          0.4064,
          0.292608
      ],
      "SourcePosition": [
          0,
          0,
          1.12
      ],
      "TablePosition": [
          0,
          0,
          0.9184810256958011
      ]
  },
  "Images": [
    {
      "ImageReference": {
        "Stream": "",
        "ImageID": 0
      }
      // Might contain additional entries like "ProjectionGeometry" (possibly incomplete):
      // Together with the ProjectionGeometries in all parent objects this should decribe the complete geometry (some values like DetectorPosition might have default values and some values like VolumeOrigin are not needed)
      "ProjectionGeometry": {
        "TableRotAngle': 0,
      }
    },
    {
      "ImageReference": {
        "Stream": "",
        "ImageID": 1
      }
      "ProjectionGeometry": {
        "TableRotAngle': 0.017453292519943295, // e.g. 2pi / 360
      }
    },
    ...
  ]
  "FirstAndLastImage": [ // optional, should contain the first and an image taken after the last image which shows the same view
    ...
  ]
}
```

The raw geometry data has an additional member, `FirstImageID`.

ConeBeamCTRepeat0
-----------------

`ConeBeamCTRepeat0` indicates a scan where the initial image was repeated
several times to be able to detect shift of the object or the focal spot.

JSON data is simlar to `ConeBeamCT`, but with additional members:

```
{
  ...
  "RepeatInitialImageEvery": 20, // optional
  "MovementReferenceImages": [
    // A list of images, all showing the same view
    { "ImageReference": { "ImageID": 0, ... }, ... },
    { "ImageReference": { "ImageID": 21, ... }, ... },
    { "ImageReference": { "ImageID": 43, ... }, ... },
    { "ImageReference": { "ImageID": 65, ... }, ... },
    ...
  ]
  "RotationCheckImages": [ // optional
    // pairs of images showing the same view before / after taking the movement reference image
    { "ImageReference": { "ImageID": 20, ... }, ... },
    { "ImageReference": { "ImageID": 22, ... }, ... },

    { "ImageReference": { "ImageID": 42, ... }, ... },
    { "ImageReference": { "ImageID": 44, ... }, ... },

    { "ImageReference": { "ImageID": 64, ... }, ... },
    { "ImageReference": { "ImageID": 66, ... }, ... },

    ...
  ]
}
```

The raw geometry data has an additional member, `FirstImageID`.

MovementReferenceData
---------------------

Reference data for object / focal spot movement detection.

Currently there is no corresponding raw geometry data.

Example JSON info:

```
{
  "Sequences": [
    {
      "Images": [
        // A list of images, all showing the same view
        { "ImageReference": { "ImageID": 0, ... }, ... },
        { "ImageReference": { "ImageID": 21, ... }, ... },
        { "ImageReference": { "ImageID": 43, ... }, ... },
        { "ImageReference": { "ImageID": 65, ... }, ... },
        ...
      ],
      // Probably geometry data for this sequence
    },
    ... // possibly other sequences showing different views
  ]
}
```
