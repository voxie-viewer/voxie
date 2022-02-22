This directory contains marching cubes test cases from the paper "Practical considerations on the topological correctness of Marching Cubes 33" by Lis Custodio, Tiago Etiene, Sinesio Pesco and Claudio Silva.

See <https://doi.org/10.1016/j.cag.2013.04.004> for the paper and <http://liscustodio.github.io/C_MC33/> for the testcases.

The files in this directory are based on the gh-pages commit b8fda4e6c1ceaee56447ad62536d278152337e67 in <https://github.com/liscustodio/C_MC33>. The files were repacked as .tar.xz files to save space.

The files are the same as in the paper's first and second supplementary data files.

```
wget https://liscustodio.github.io/C_MC33/Closed_Surfaces.zip
rm -rf Closed_Surfaces
unzip -q Closed_Surfaces.zip Closed_Surfaces/\*
find Closed_Surfaces -name .DS_Store -delete
find "Closed_Surfaces" -type f -print0 | LC_ALL=C sort -z | tar cJf "Closed_Surfaces.tar.xz" --owner=0 --group=0 --no-recursion --null -T -
rm -rf Closed_Surfaces
rm -f Closed_Surfaces.zip
```

```
wget https://liscustodio.github.io/C_MC33/MarchingCubes_cases.zip
rm -rf MarchingCubes_cases
unzip -q MarchingCubes_cases.zip MarchingCubes_cases/\*
find MarchingCubes_cases -name .DS_Store -delete
find "MarchingCubes_cases" -type f -print0 | LC_ALL=C sort -z | tar cJf "MarchingCubes_cases.tar.xz" --owner=0 --group=0 --no-recursion --null -T -
rm -rf MarchingCubes_cases
rm -f MarchingCubes_cases.zip
```
