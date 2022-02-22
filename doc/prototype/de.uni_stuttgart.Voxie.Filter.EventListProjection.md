## Description

This filter accepts a clustered event stream produced by the
[voxie:///help/prototype/de.uni_stuttgart.Voxie.Filter.EventListClustering](Event List Clustering Filter).

It transforms this event stream into a Raw Tomography Data object, a stack of 2D images, each representing a distinct stream within the imported `.timepixraw.json` file.

The **Projection Attribute** property determines which attribute of each event should be projected. This can be *Energy* (in keV), *Event count* and *Time of arrival* (in seconds since scan start time).

The **Accumulation Mode** property decides how multiple events landing on the same pixel should be aggregated: available modes are *Sum*, *Mean*, *Maximum* and *Minimum*. Note that if *Event count* is chosen as the projection attribute, only the *Sum* setting will produce meaningful results, as each event has an inherent "count" of 1.

Energy range thresholds (in keV) can be configured via the **Minimum Energy** and **Maximum Energy** properties to filter the projected image to a specific spectrum of cluster energies.

Likewise, a time threshold in seconds can be configured via **Minimum Timestamp** and **Maximum Timestamp**, limiting the projection to a specific time range and thus reducing the processing workload as only a portion of the dataset has to be processed.

The size of the output image in pixels is given by the properties **Image Width** and **Image Height**. A larger size compared to the detectors' total pixel area results in subpixel-accurate positions for clusters spanning multiple pixels. However, for images mostly comprised of smaller clusters, this will generally result in a grid-like appearance of the resulting dataset, with most clusters aliging to integer pixel coordinates.

This filter operates on the principle of *lazy-loading*.
Its execution via the "Calculate" button within Voxie produces an output data object immediately.
Once a specific section of the output data object is accessed, the filter's actual projection logic runs.
