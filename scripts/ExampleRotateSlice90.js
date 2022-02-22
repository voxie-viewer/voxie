/*
 * Copyright (c) 2014-2022 The Voxie Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

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
