DBus data stored as JSON
========================

(Note: this is basically the opposite of [json-on-dbus](voxie:///help/topic/interfaces/json-on-dbus)).

This is a set of rules for storing almost arbitrary DBus data as JSON.

- `s` is stored as a string
- `b` is stored as a boolean
- All integers are stored as a number
- `d` is stored as a number. If the value is not finite, the strings `"NaN"`, `"Infinity"` and `"-Infinity"` are used.
- `a?` is stored as an array
- `(...)` is also stored as an array (with a static size)
- `a{s?}` is stored as an object (arrays with different keys currently aren't supported)
- `g` is stored as a string

For variants, there are two possible ways of storing the data:

- For supporting arbitrary variants: `v` is stored as an array with two values, the first is a string with the signature of the content, the second is the content itself.
- For JSON data: `v` is always assumed to be [json-on-dbus](voxie:///help/topic/interfaces/json-on-dbus) data and is simply stored as the JSON data itself.

Support for object paths (`o`) is optional, if it is supported, it is stored as a string. (In most cases, storing object paths does not make sense because object paths only have meaning as long as the program is runnung.)
