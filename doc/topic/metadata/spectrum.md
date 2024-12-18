Spectrum metadata
=================

Spectrum metadata is a spectral weight function $`w(E)`$ which describes how
the photon energies $`E`$ are weighted in the data.

Raw tomography data
-------------------

For raw tomography data (whether in intensity or attenuation domain) the
spectral weight function describes the effective spectrum used for the
acqisition.

- $`w(E)`$ is the weight function
- $`E`$ is the photon energy in the unit $`eV`$
- $`H_{e,E}`$ is the spectral exposure arriving at the detector (without any object or filter between the source and the detector) at photon energy $`E`$ in the unit $`J \cdot m^{-2} \cdot eV^{-1}`$
- $`f_E`$ is the attenuation of photon energy $`E`$ through filtering on the way to the detector, which is dimensionless
- $`A`$ is the area of a pixel in the unit $`m^2`$
- $`K_e(E)`$ is the efficacy at photon energy $`E`$ in the unit $`J^{-1}`$. (This assumes that the values read out from the detector are dimensionless.) For monochromatic radiation with photon energy $`E`$, this value is the value read out from the detector (after dark field correction) divided by the radiant exposure hitting the detector, or the reciprocal of the energy needed to increase the detector value by $`1`$.
- $`I_0`$ is the intensity read out from the detector without any object between the source and the detector (but with filtering)
- $`c`$ is an arbitrary constant

```math
I_0 = \int_0^{\infty} H_{e,E} \cdot e^{-f_E} \cdot A \cdot K_e(E) \ dE
```

The weight function can then be calculated as ($`c`$ can be chosen arbitrarily):

```math
w(E) = H_{e,E} \cdot e^{-f_E} \cdot A \cdot K_e(E) \cdot c
```

Notes:
- $`w(E)`$ should be the same for all detector pixels, which means that the value of $`c`$ can be different for different detector pixels (because the spectral exposure $`H_{e,E}`$ of different pixels will differ because of the different distance and angle). Another effect is that the value $`f_E`$ will differ because of the different lengths of the air path (and possibly different lengths of the path though other filters), but this will cause a different spectrum for different pixels, which will simply be ignored here.
- When $`H_{e,E}`$ is replaced by $`H_{p,E}`$ (the spectral photon exposure, unit $`m^{-2} \cdot eV^{-1}`$) and $`K_e(E)`$ is replaced by $`K_p(E)`$ (the reciprocal of the number of photons with energy $`E`$ needed to increase the detector value by $`1`$, dimensionless), everything will stay the same. ($`H_{p,E} = \frac{H_{e,E}}{E}`$, $`K_p(E) = E \cdot K_e(E)`$)

Volume data
-----------

The spectral weight function of a volume describes how the attenuation
coefficients $`\mu`$ which is recorded in the volume data can be calculated
from the material parameters $`\mu_E`$.

```math
\mu = \frac{ \int_0^{\infty} \mu_E \cdot w(E) \ dE }{ \int_0^{\infty} w(E) \ dE }
```

Here, $`\mu`$ is the attenuation value recorded in the volume (in $`m^{-1}`$),
$`\mu_E`$ is the attenuation coefficient of the material at photon energy $`E`$
and $`w(E)`$ is the spectral weight function.

Reconstruction
--------------

Assuming no beam hardening is happening, a volume reconstructed from a raw data
set with a certain spectral weight function has the same spectral weight
function.

In this section the following variables are used:
- $`l`$ is the thickness of the material, i.e. the length along which the beam is being attenuated
- $`\mu_E`$ is the $`\mu`$ value of the material at photon energy $`E`$, normally taken from the NIST tables
- $`\mu_{act}`$ is the $`\mu`$ value which is actually being measured
- $`\mu_{nobh}`$ is the $`\mu`$ value which would be measured if there was no beam hardening
- $`I`$ is the intensity read out from the detector with the object between the source and the detector

When the initial detector intensity of a pixel is $`I_0`$ and the detector
intensity after passing through a mono-material object of length $`l`$ is $`I`$,
the reconstruction will assign this material an attenuation coefficient of
the following value $`\mu_{act}`$:

```math
\mu_{act} = - \frac{1}{l} \cdot \ln \frac{I}{I_0}
```

For $`I_0`$ see above, $`I`$ is similar but also includes a term $`e^{-l\mu_E}`$
for the attenuation through the material.

```math
\mu_{act} = - \frac{1}{l} \cdot \ln \frac{ \int_0^{\infty} H_{e,E} \cdot e^{-f_E} \cdot e^{-l\mu_E} \cdot A \cdot K_e(E) \ dE }{ \int_0^{\infty} H_{e,E} \cdot e^{-f_E} \cdot A \cdot K_e(E) \ dE }
```

When using the definition of $`w(E)`$:

```math
\mu_{act} = - \frac{1}{l} \cdot \ln \frac{ \int_0^{\infty} w(E) \cdot \frac{1}{c} \cdot e^{-l\mu_E} \ dE }{ \int_0^{\infty} w(E) \cdot \frac{1}{c} \ dE }
```

```math
\mu_{act} = - \frac{1}{l} \cdot \ln \frac{ \int_0^{\infty} w(E) \cdot e^{-l\mu_E} \ dE }{ \int_0^{\infty} w(E) \ dE }
```

The strength of beam hardening depends on $`l`$. The beam hardening free case is
the limit when $`l`$ goes to $`0`$:

```math
\mu_{nobh} = \lim_{l \to 0} \left( - \frac{1}{l} \cdot \ln \frac{ \int_0^{\infty} w(E) \cdot e^{-l\mu_E} \ dE }{ \int_0^{\infty} w(E) \ dE } \right)
```

The following function $`f`$ can be defined:

```math
f(l) = - \ln \frac{ \int_0^{\infty} w(E) \cdot e^{-l\mu_E} \ dE }{ \int_0^{\infty} w(E) \ dE }
```

```math
f(0) = - \ln \frac{ \int_0^{\infty} w(E) \cdot e^{-0\mu_E} \ dE }{ \int_0^{\infty} w(E) \ dE } = - \ln \frac{ \int_0^{\infty} w(E) \ dE }{ \int_0^{\infty} w(E) \ dE } = - \ln 1 = 0
```

$`\mu_{nobh}`$ can now be rewritten as:

```math
\mu_{nobh} = \lim_{l \to 0} \frac{ - \ln \frac{ \int_0^{\infty} w(E) \cdot e^{-l\mu_E} \ dE }{ \int_0^{\infty} w(E) \ dE } }{l}
```

```math
\mu_{nobh} = \lim_{l \to 0} \frac{ f(l) }{l}
```

```math
\mu_{nobh} = \lim_{l \to 0} \frac{ f(0 + l) - 0 }{l}
```

```math
\mu_{nobh} = \lim_{l \to 0} \frac{ f(0 + l) - f(0) }{l}
```

```math
\mu_{nobh} = f'(0)
```

I.e. the value of $`\mu_{nobh}`$ is the derivative of $`f`$ at position $`0`$.
Consider the derivation of $`f`$:

```math
f(l) = - \ln \frac{ \int_0^{\infty} w(E) \cdot e^{-l\mu_E} \ dE }{ \int_0^{\infty} w(E) \ dE }
```

```math
f'(l) = - \frac{1}{ \frac{ \int_0^{\infty} w(E) \cdot e^{-l\mu_E} \ dE }{ \int_0^{\infty} w(E) \ dE } } \cdot \frac{ \int_0^{\infty} w(E) \cdot e^{-l\mu_E} \cdot (-\mu_E) \ dE }{ \int_0^{\infty} w(E) \ dE }
```

```math
f'(l) = - \frac{ \int_0^{\infty} w(E) \ dE }{ \int_0^{\infty} w(E) \cdot e^{-l\mu_E} \ dE } \cdot \frac{ \int_0^{\infty} w(E) \cdot e^{-l\mu_E} \cdot (-\mu_E) \ dE }{ \int_0^{\infty} w(E) \ dE }
```

```math
f'(l) = - \frac{ \int_0^{\infty} w(E) \cdot e^{-l\mu_E} \cdot (-\mu_E) \ dE }{ \int_0^{\infty} w(E) \cdot e^{-l\mu_E} \ dE }
```

When plugging in $`l = 0`$:

```math
\mu_{nobh} = f'(0) = - \frac{ \int_0^{\infty} w(E) \cdot e^{-0\mu_E} \cdot (-\mu_E) \ dE }{ \int_0^{\infty} w(E) \cdot e^{-0\mu_E} \ dE }
```

```math
\mu_{nobh} = - \frac{ \int_0^{\infty} w(E) \cdot (-\mu_E) \ dE }{ \int_0^{\infty} w(E)  \ dE }
```

```math
\mu_{nobh} = \frac{ \int_0^{\infty} \mu_E \cdot w(E) \ dE }{ \int_0^{\infty} w(E)  \ dE }
```

This matches the definition of $`w(E)`$ for the volume, i.e. in the beam
hardening free case the reconstruction result should match exactly the expected
$`\mu`$ value and the same spectral weight function which was used for the
tomography raw data can be used for the volume.
