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

#ifndef MULTIMATERIALVOLUMESEGMENTATION_H
#define MULTIMATERIALVOLUMESEGMENTATION_H

#include <VoxieClient/Array.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <future>
#include <iostream>
#include <list>
#include <numeric>
#include <sstream>
#include <vector>

#include "Voxel.hpp"

class MultiMaterialVolumeSegmentation {
 public:
  MultiMaterialVolumeSegmentation(vx::Array3<const float> inputVolume,
                                  vx::Array3<uint8_t> outputVolume,
                                  std::string thresholds, size_t iterations,
                                  float thresholdDifferenceSignificance,
                                  float epsilon);

  void run();

  // Remember to modify default values in json when altering these constants for
  // more transparency
  const std::string DEFAULT_THRESHOLDS = "0.33; 1.23; 4.20";
  const size_t DEFAULT_ITERATIONS = 10;
  const float DEFAULT_DIFFERENCE_SIGNIFICANCE = 0.05f;
  const float DEFAULT_EPSILON = 10000;

 private:
  size_t thresholdCount;
  size_t iterations;  // paper says algorithm usually converges at 10 iterations
  float thresholdDifferenceSignificance;  // user input, determines iteration
                                          // count
  float epsilon;  // user input, guesstimated rough value that needs refinement
                  // // currently at infinity because of insufficient gradient
                  // calculation + reduction of necessary sample size

  vx::Array3<Voxel> voxels;

  std::vector<float> initialThresholds;  // user input
  std::vector<float> thresholds;
  std::vector<float> oldThresholds;

  std::vector<float> meanValues;
  std::vector<float> stdDevValues;

  std::vector<std::vector<float>> thresholdRanges;
  float grayMin = INFINITY;
  float grayMax = -INFINITY;

  void convertToVoxels(vx::Array3<const float>&);
  void convertToOutputVolume(vx::Array3<uint8_t>&);
  std::vector<float> parseThresholds(std::string input);
  void calculateLabelChanges(Voxel& doubtVoxel, size_t x, size_t y, size_t z);
  int8_t adaptiveThresholding(Voxel& g);

  size_t iterationCounter = 0;  // only used for testing
  size_t start = 200;           // only used for testing
  size_t stop = start + 100;    // only used for testing
};

#endif  // MULTIMATERIALVOLUMESEGMENTATION_H
