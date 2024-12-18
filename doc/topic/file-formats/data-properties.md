Data proeprties
===============

Data properties provide metadata for a file.

For file formats where the main file is a JSON file, data properties are
normally stored in the `"DataProperties"` member of the root object.
If this member is missing, there are no data properties in the file.

If it exists, the `"DataProperties"` member is an object with one entry for each
data property. The key is used as the name of the property, the `"Value"` entry
is used as the value, all other entries are passed to the `CreateDataProperty()`
call `json` parameter. The value itself is encoded as
[dbus-as-json](voxie:///help/topic/interfaces/dbus-as-json).

An example for the JSON file:

```
{
  ... Other values
  "DataProperties": {
    "de.uni_stuttgart.Voxie.DataProperty.Time": {
      "DisplayName": "Time",
      "Type": "de.uni_stuttgart.Voxie.PropertyType.Float",
      "Value": 2.13
    },
    "de.uni_stuttgart.Voxie.DataProperty.MiscInformation": {
      "DisplayName": "Misc",
      "Type": "de.uni_stuttgart.Voxie.PropertyType.String",
      "Value": "Hello world"
    }
  },
  ... Other values
}
```
