JSON data on DBus
=================

(Note: this is basically the opposite of [dbus-as-json](voxie:///help/topic/interfaces/dbus-as-json)).

Encoding of JSON data as DBus Variants:

- Objects are encoded as `a{sv}`
- Arrays are encoded as `av`
- Strings are encoded as `s`
- Integers are encoded as `x` if possible, otherwise as `t`
- Floating point numbers are encoded as `d`
- Boolean values are encoded as `b`
- null values are encoded as a DBus signature `g` containing an empty string
