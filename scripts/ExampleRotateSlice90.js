var angle = 90
var axis = [ 0, 0, 1 ]

var slice = voxie.Gui.ActiveVisualizer.Slice
var plane = slice.Plane
var sin = Math.sin ((angle / 180 * Math.PI) / 2)
var cos = Math.cos ((angle / 180 * Math.PI) / 2)
var a = plane.Rotation
var b = [ cos, sin * axis[0], sin * axis[1], sin * axis[2] ]
// Quaternion multiplication
var c = [a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3],
         a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2],
         a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1],
         a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0]]
plane.Rotation = c
slice.Plane = plane
