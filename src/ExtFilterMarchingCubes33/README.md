**Implementation**

This Marching Cubes implementation is incomplete. It misses the resolving of the ambiguous main cases 12 and 13 as described in [this paper](https://cds.cern.ch/record/292771/files/cn-95-017.pdf) and the correct tunnel orientation for the sub case 13.5 and non-manifold surface resolving as described in [this paper](https://www.sciencedirect.com/science/article/pii/S0097849313000563).

---

**MarchingCubes.blend**

The MarchingCubes.blend file contains the surfaces for all ambiguous Marching Cubes cases in a voxel as well as a numbering reference for the voxel corners and edges. For the numbering reference you need to install the MeasureIt addon (Edit -> Preferences -> Search: MeasureIt) then in the main Blender window open the side menu ('n'-key) navigate to the view tab and under 'MeasureIt Tools' click 'Show'.

Please do not open the blend file here but copy-paste it somewhere else and remove the last part (.ReadTheREADME). This is because the file is a binary file and Git can therefor not handle it well meaning if anything changes (which happens by simply interacting with it) the whole file is updated leading to unneccessary bloat in the commits and by extension the repository. If you need to update it make sure to combine all updates you want to make into one to reduce bloat.

---

**CubeIndices.txt**

The CubeIndices.txt file contains all 256 possible corner configurations of a voxel (which corners are above and below a threshold) sorted by their Marching Cubes main case. In the actual implementation they are converted to hexadecimal numbers.
