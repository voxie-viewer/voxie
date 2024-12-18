ChemicalComposition property type
=================================

The property type
`de.uni_stuttgart.Voxie.PropertyType.ChemicalComposition` describes the chemical
composition of the material whose density is described by the current volume.

DBus signature
--------------

The DBus signature of the type is `v`.

The data is a JSON value (see
[json-on-dbus](voxie:///help/topic/interfaces/json-on-dbus) which describes the
chemical composition. The JSON value is an array where the first value describes
the type of the ChemicalComposition.

ChemicalComposition types
-------------------------

### Element

Elements have the string `"element"` as the first element. The second element
contains the atomic number `Z`, the third optional element the nucleon number
`A` which defaults to `null`.

Examples:
```
# Oxygen
["element", 8]

# Oxygen, same as previous one
["element", 8, null]

# Deuterium
["element", 1, 2]
```

### Molecule

Molecules have the string `"molecule"` as the first element. The second element
is an array of components, where each component is an array with two entries:
The weight (integer or float) and the ChemicalComposition of the component.
All ChemicalComposition values must be `"element"` or `"molecule"` entries which
may be nested into `"meta"` entries.

Examples:
```
# Water (H2O)
["molecule", [[2, ["element", 1]], [1, ["element", 8]]]]

# Nitrogen (N2)
["molecule", [[2, ["element", 7]]]]
```

### Compound

Compounds have the string `"compound"` as the first element.
The second element is a string which is either `"amount_of_substance"`, `"mass"`
or `"volume"` and which describes the dimension of the fractions.
The third is an array of components, where each components is an array with two
entries: The fractions (integer or float) and the ChemicalComposition of the
component. The sum of the fractions does not have to be one (they will be
normalized if they are not).

Examples:
```
# Air
["compound", "amount_of_substance", [
  [78, ["molecule", [[2, ["element", 7]]]]],
  [21, ["molecule", [[2, ["element", 8]]]]],
  [1, ["element", 18]]
]]
```

### Meta information

Meta information elements have the string `"meta"` as the first element.
The second element is a ChemicalComposition value. The third element is a JSON
object which contains metadata.

Possible metadata keys are:
- `"Description"`: A string which describes the material.
- `"WikidataEntityID"`: A string which contains the wikidata entity ID for the material, e.g. `"Q283"`.
- `"Density"`: A floating point number describing the density of the material in kg/m³.
- `"MolarMass"`: A floating point number describing the molar mass of the material in kg/mol.
- `"MolarVolume"`: A floating point number describing the molar mass of the material in m³/mol.

At most two of `"Density"`, `"MolarMass"` and `"MolarVolume"` may be given.

Examples:
```
# Air (molar volume from ideal gas at 1 bar / 25 °C)
["meta",
  ["compound", "amount_of_substance", [
    [78, ["molecule", [[2, ["element", 7]]]]],
    [21, ["molecule", [[2, ["element", 8]]]]],
    [1, ["element", 18]]
  ]
], {
  "Description": "Air",
  "WikidataEntityID": "Q3230",
  "MolarVolume": 0.024789570296023
}]
```
