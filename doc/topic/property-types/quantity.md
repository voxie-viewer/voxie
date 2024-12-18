Quantity property type
======================

The property type `de.uni_stuttgart.Voxie.PropertyType.Quantity` describes the
kind of a (physical) quantity, i.e. whether some value describes a length or a
mass or something else.

This is what is called 'quantity' or 'general quantity' in ISO 80000-1.

DBus signature
--------------

The DBus signature of the type is `s`.

The data is one of these values:

- `de.uni_stuttgart.Voxie.Quantity.AttenuationCoefficient`: The (linear) attenuation coefficient, SI unit is 1/m
- `de.uni_stuttgart.Voxie.Quantity.Density`: The volumetric mass density, SI unit is kg/m³
- `de.uni_stuttgart.Voxie.Quantity.EffectiveAtomicNumber`: The effective atomic number of some material, SI unit is 1. Can be calculated according to different rules.
