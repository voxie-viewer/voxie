## Description

This filter accepts a raw event stream in `.t3r` or `.timepixraw.json` format loaded by the Event List Importer.

It transforms this event stream into a *clustered* version, grouping activation events together
if they are sufficiently close to each other in time and space.

The output event stream differs from the input event stream in multiple ways:

- Event positions are no longer stored as pixel integer coordinates from 0 to 255, but instead as floating point coordinates representing the weighted center of mass of each cluster.
- Each group of sub-streams within an event stream is stitched together into a single event stream. The events can be distinguished by the transformation applied to them, based on the `Origin`, `Rotation` and `PixelSize` properties in the `.timepixraw.json` file. For instance, a 4-detector dataset with 200 `.t3r` files in total will be reduced to 50 images by this filter.
- Each event has a floating-point `energy` attribute, representing the sum of energies corresponding to each event's *Time over Threshold* (ToT) within a cluster, in units of kilo-electron-Volt. The original *ToT* values are discarded.
- Clusters that are deemed to be the result of the *X-Ray Fluourescence* effect are removed. This step can be configured or disabled by toggling the *XRF Correction* parameter or tweaking its settings. The energy of each pair of clusters detected by *XRF Correction* is merged into the cluster with a higher energy level, with the lower-energy cluster being removed.

This filter operates on the principle of *lazy-loading*.
Its execution via the "Calculate" button within Voxie produces an output data object immediately.
Once a specific section of the output data object is accessed, whether directly or indirectly, the filter's actual clustering logic runs.
