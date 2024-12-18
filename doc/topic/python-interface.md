Voxie Python Interface
======================

Useful commands
---------------

The following code snippets can be used e.g. in project files.

### Running all filters

The following code will run all filters where the parameters / input data
has changed.

```
with instance.RunAllFilters() as result:
    result.Operation.WaitFor()
```

### Running a single filter

The following code will one filter and wait until it finishes. In project
files, `node` can be taken from one of the return values of
`instance.CreateNodeChecked()`, interactively it can be taken e.g. from
`instance.Gui.SelectedNodes[0]`.

```
with node.CastTo('de.uni_stuttgart.Voxie.FilterNode').RunFilter() as result:
    result.Operation.WaitFor()
```

### Exporting a data node

The following code will export a (volume) data node as a .vxvol.json file.

```
exporter = instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.Exporter', 'de.uni_stuttgart.Voxie.FileFormat.Volume.VxVol.Export').CastTo('de.uni_stuttgart.Voxie.Exporter')
with exporter.StartExport(node.CastTo('de.uni_stuttgart.Voxie.DataNode').Data, 'out.vxvol.json') as result:
    result.Operation.WaitFor()
```

Interactive use
---------------

An interactive python console can be opened with Ctrl+T or Ctrl+S.

### Getting a node object

To get a node object, select the node and run:

```
node = instance.Gui.SelectedNodes[0]
```
