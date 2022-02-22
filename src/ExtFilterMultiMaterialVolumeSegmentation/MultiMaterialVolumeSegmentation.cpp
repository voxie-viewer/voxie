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

#include "MultiMaterialVolumeSegmentation.hpp"

MultiMaterialVolumeSegmentation::MultiMaterialVolumeSegmentation(
    vx::Array3<const float> inputVolume, vx::Array3<uint8_t> outputVolume,
    std::string thresholds, size_t specifiedIterations,
    float specifiedThresholdDifferenceSignificance, float specifiedEpsilon)
    : voxels({inputVolume.size<0>(), inputVolume.size<1>(),
              inputVolume.size<2>()}) {
  this->thresholdCount = parseThresholds(thresholds).size();
  this->initialThresholds = parseThresholds(thresholds);
  if (specifiedIterations == 0)
    this->iterations = DEFAULT_ITERATIONS;
  else
    this->iterations = specifiedIterations;

  if (specifiedThresholdDifferenceSignificance == 0.0f)
    this->thresholdDifferenceSignificance = DEFAULT_DIFFERENCE_SIGNIFICANCE;
  else
    this->thresholdDifferenceSignificance =
        specifiedThresholdDifferenceSignificance;

  if (specifiedEpsilon == 0.0f)
    this->epsilon = DEFAULT_EPSILON;
  else
    this->epsilon = specifiedEpsilon;

  convertToVoxels(inputVolume);
  convertToOutputVolume(outputVolume);
}

void MultiMaterialVolumeSegmentation::convertToVoxels(
    vx::Array3<const float>& inputVolume) {
  for (size_t z = 0; z < inputVolume.size<2>(); z++) {
    for (size_t y = 0; y < inputVolume.size<1>(); y++) {
      for (size_t x = 0; x < inputVolume.size<0>(); x++) {
        voxels(x, y, z) = Voxel(inputVolume(x, y, z));
      }
    }
  }
}

void MultiMaterialVolumeSegmentation::convertToOutputVolume(
    vx::Array3<uint8_t>& labels) {
  // Insert material labels of all voxels into outputVolume
  for (size_t z = 0; z < voxels.size<2>(); z++) {
    for (size_t y = 0; y < voxels.size<1>(); y++) {
      for (size_t x = 0; x < voxels.size<0>(); x++) {
        labels(x, y, z) = voxels(x, y, z).getFirstLabel();
      }
    }
  }
}

std::vector<float> MultiMaterialVolumeSegmentation::parseThresholds(
    std::string input) {
  // ensures that '.' is interpreted as the decimal separator
  std::locale::global(std::locale("en_US.UTF8"));
  std::cout.imbue(std::locale());

  std::vector<float> thresholds;
  if (input != "") {
    std::replace(input.begin(), input.end(), ',', '.');
    std::stringstream split(input);
    while (split.good()) {
      std::string value;
      std::getline(split, value, ';');
      std::cout << value;
      std::cout << " ";
      std::cout << std::stof(value);
      std::cout << "\n";
      thresholds.push_back(std::stof(value));
    }
  }

  if (thresholds.size() == 0) {
    return parseThresholds(DEFAULT_THRESHOLDS);
  } else {
    return thresholds;
  }
}

void MultiMaterialVolumeSegmentation::calculateLabelChanges(Voxel& doubtVoxel,
                                                            size_t x, size_t y,
                                                            size_t z) {
  if ((x < voxels.size<0>()) && (y < voxels.size<1>()) &&
      (z < voxels.size<2>())) {
    for (uint8_t i = 0; i < Voxel::MAX_LABELS; i++) {
      if (voxels(x, y, z).getLabels()[i]) doubtVoxel.addLabel(i);
    }
  }
}

int8_t MultiMaterialVolumeSegmentation::adaptiveThresholding(Voxel& g) {
  // equal to algorithm in appendix of "Multi-material Volume Segmentation for
  // Isosurfacing Using Overlapped Label Propagation"
  int8_t label = -1;
  if (g.getLabelCount() >=
      2)  // unlabelled voxels will result in material -1 as an error state
  {
    uint8_t i = 0;
    uint8_t k;
    while (i < meanValues.size() - 1) {
      k = i + 1;
      if (g.getLabels()[i]) {
        while (!g.getLabels()[k] && k < meanValues.size() - 1) {
          k++;
        }
        if (g.getGrayValue() < (meanValues.at(i) + meanValues.at(k)) / 2) {
          label = i;
          break;
        }
      }
      i = k;
      if (i == meanValues.size() - 1) {
        label = i;
      }
    }
  } else {
    label = g.getFirstLabel();
  }
  return label;
}

void MultiMaterialVolumeSegmentation::run() {
  // Get min and max gray values
  std::future<void> minMaxFuture = std::async([&]() {
    for (size_t z = 0; z < voxels.size<2>(); z++) {
      for (size_t y = 0; y < voxels.size<1>(); y++) {
        for (size_t x = 0; x < voxels.size<0>(); x++) {
          if (voxels(x, y, z).getGrayValue() < grayMin)
            grayMin = voxels(x, y, z).getGrayValue();
          if (voxels(x, y, z).getGrayValue() > grayMax)
            grayMax = voxels(x, y, z).getGrayValue();
        }
      }
    }
  });

  thresholds = initialThresholds;
  oldThresholds = thresholds;

  // Compute and refine thresholds over multiple iterations
  for (size_t i = 0; i < iterations; i++)  // iterations of refinement
  {
    // Preparation for next iteration
    std::sort(thresholds.begin(), thresholds.end());
    float lastThreshold = -INFINITY;
    thresholds.push_back((float)INFINITY);

    // Remove non-final material classification labels from voxels for next
    // iteration
    for (size_t z = 0; z < voxels.size<2>(); z++) {
      for (size_t y = 0; y < voxels.size<1>(); y++) {
        for (size_t x = 0; x < voxels.size<0>(); x++) {
          voxels(x, y, z).clearLabels();
        }
      }
    }

    // sort voxels relative to their gray values relative to threshold values
    for (size_t j = 0; j < thresholdCount + 1; j++) {
      for (size_t z = 0; z < voxels.size<2>(); z++) {
        for (size_t y = 0; y < voxels.size<1>(); y++) {
          for (size_t x = 0; x < voxels.size<0>(); x++) {
            if (lastThreshold < voxels(x, y, z).getGrayValue() &&
                thresholds.at(j) >= voxels(x, y, z).getGrayValue()) {
              // voxels with grey value above last and below current threshold
              // are labelled with j
              voxels(x, y, z).addLabel(j);
            }
          }
        }
      }
      lastThreshold = thresholds.at(j);
    }
    thresholds.pop_back();  // remove temporary INFINITY-value at end of list
                            // only used as unreachable upper boundary

    std::cout << "Voxels sorted (";
    std::cout << i;
    std::cout << ")\n";

    std::cout << "Voxel Sets (";
    std::cout << i;
    std::cout << "):\n";

    // Compute mean values of each voxel set
    bool materialEmpty = false;
    std::vector<std::future<void>> meanFutures;
    meanFutures.reserve(thresholdCount + 1);
    std::vector<float> oldMeanValues = meanValues;  // Failsafe
    meanValues.clear();
    for (uint8_t m = 0; m < thresholdCount + 1; m++) {
      meanFutures.push_back(std::async(
          [&](uint8_t material) {
            float sum = 0.0f;
            uint64_t voxelsPerMaterial = 0;
            for (size_t z = 0; z < voxels.size<2>(); z++) {
              for (size_t y = 0; y < voxels.size<1>(); y++) {
                for (size_t x = 0; x < voxels.size<0>(); x++) {
                  if (voxels(x, y, z).getFirstLabel() ==
                      material)  // every voxel has 1 label at this point
                  {
                    sum += voxels(x, y, z).getGrayValue();
                    ;
                    voxelsPerMaterial++;
                  }
                }
              }
            }
            if (voxelsPerMaterial == 0) materialEmpty = true;
            float mean = sum / voxelsPerMaterial;
            meanValues.push_back(mean);
          },
          m));
    }

    std::cout << "Waiting for MeanValues (";
    std::cout << i;
    std::cout << ")\n";
    for (auto& future : meanFutures) future.wait();
    meanFutures.clear();
    if (materialEmpty) {
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.ExtFilterMultiMaterialVolumeSegmentation."
          "Error",
          "bad threshholds, material without voxels");
    }
    if (i != 0 &&
        meanValues.size() !=
            thresholdCount + 1)  // Failsafe, should never actually be true
    {
      i = iterations - 1;
      meanValues = oldMeanValues;
    }
    std::sort(meanValues.begin(), meanValues.end());

    std::cout << "Mean values computed (";
    std::cout << i;
    std::cout << +")\n";

    // Compute new thresholds and add them to list
    thresholds.clear();
    for (uint8_t j = 0; j < meanValues.size() - 1; j++) {
      thresholds.push_back((meanValues.at(j) + meanValues.at(j + 1)) / 2);
    }
    std::sort(thresholds.begin(), thresholds.end());

    // Check if further computation is necessary (did thresholds change?)
    bool thresholdsDidNotChange = true;
    int c = 0;
    for (float threshold : thresholds) {
      thresholdsDidNotChange &= (std::abs(oldThresholds.at(c) - threshold) <=
                                 thresholdDifferenceSignificance);
      c++;
    }
    if (thresholdsDidNotChange || i == iterations - 1) {
      iterationCounter = (int)(i + 1);
      i = iterations - 1;  // jump to last iteration
    }

    // Compute standard deviation values of each voxel set
    if (i == iterations - 1) {
      stdDevValues.clear();
      std::map<uint8_t, float> stdDevPairs;
      std::vector<std::future<void>> stdDevFutures;
      stdDevFutures.reserve(thresholdCount + 1);
      for (uint8_t material = 0; material < thresholdCount + 1; material++) {
        stdDevFutures.push_back(std::async(
            // TODO: This should probably not use & caputure
            [&](uint8_t material2) {
              float variance = 0.0f;
              size_t voxelsPerMaterial = 0;
              for (size_t z = 0; z < voxels.size<2>(); z++) {
                for (size_t y = 0; y < voxels.size<1>(); y++) {
                  for (size_t x = 0; x < voxels.size<0>(); x++) {
                    if (voxels(x, y, z).getLabels()[material2]) {
                      variance += ((voxels(x, y, z).getGrayValue() -
                                    meanValues.at(material2)) *
                                   ((voxels(x, y, z).getGrayValue() -
                                     meanValues.at(material2))));
                      voxelsPerMaterial++;
                    }
                  }
                }
              }
              (voxelsPerMaterial != 0) ? variance /= voxelsPerMaterial
                                       : variance = 0.0f;
              stdDevPairs.insert(
                  std::pair<uint8_t, float>(material2, std::sqrt(variance)));
            },
            material));
      }
      std::cout << "Waiting for StdDev values (";
      std::cout << i;
      std::cout << ")\n";
      for (auto& future : stdDevFutures) future.wait();
      stdDevFutures.clear();
      stdDevFutures.reserve(0);

      // Sort stdDevValues in order matching corresponding material order
      for (size_t l = 0; l < thresholdCount + 1; ++l) {
        stdDevValues.push_back(stdDevPairs.find(l)->second);
      }
    }
    std::cout << "StdDev values computed (";
    std::cout << i;
    std::cout << ")\n";

    // Debug
    std::cout << ">Current Thesholds: ";
    for (auto& th : thresholds) {
      std::cout << th;
      std::cout << ", ";
    }
    std::cout << "\n";

    oldThresholds = thresholds;
  }

  // Test: Output (1)
  std::cout << "Total Iterations: ";
  std::cout << iterationCounter;
  std::cout << "\n\nInitial Thresholds: ";
  for (float initialThreshold : initialThresholds) {
    std::cout << initialThreshold;
    std::cout << ", ";
  }
  std::cout << "\nThresholds: ";
  for (float threshold : thresholds) {
    std::cout << threshold;
    std::cout << ", ";
  }
  //    std::cout << "\n\nVoxel Sets:\n";
  //    for (size_t t = 0;  t < thresholdCount+1; t++)
  //    {
  //        std::cout << "Set ";
  //        std::cout << t;
  //        std::cout << ": ";
  //        for (size_t z = 0; z < voxels.size<2>(); z++)
  //        {
  //            for (size_t y = 0; y < voxels.size<1>(); y++)
  //            {
  //                for (size_t x = 0; x < voxels.size<0>(); x++)
  //                {
  //                    if (voxels(x, y, z).getLabels()[0] == t)
  //                    {
  //                        std::cout << voxels(x, y, z).getGrayValue();
  //                        std::cout << ", ";
  //                    }
  //                }
  //            }
  //        }
  //        std::cout << "\n";
  //    }
  std::cout << "\nMean Values: ";
  for (float meanValue : meanValues) {
    std::cout << meanValue;
    std::cout << ", ";
  }
  std::cout << "\nStandard Deviation Values: ";
  for (float stdDevValue : stdDevValues) {
    std::cout << stdDevValue;
    std::cout << ", ";
  }
  std::cout << "\n";

  // Remove non-final material classification labels from voxels that were only
  // used for calculating threshold values
  for (size_t z = 0; z < voxels.size<2>(); z++) {
    for (size_t y = 0; y < voxels.size<1>(); y++) {
      for (size_t x = 0; x < voxels.size<0>(); x++) {
        voxels(x, y, z).clearLabels();
      }
    }
  }

  // Compute threshold ranges with
  std::cout << "Waiting for Min/Max\n";
  minMaxFuture.wait();
  std::cout << "Min/Max done!\n";
  thresholdRanges.push_back(std::vector<float>({grayMin, thresholds.front()}));
  for (size_t i = 1; i < thresholdCount; i++) {
    //        if (i == 0)
    //        {
    //            thresholdRanges.push_back(std::vector<float>({ grayMin,
    //            thresholds.front()}));
    //        }
    //        else if (i < thresholdCount)
    //        {
    thresholdRanges.push_back(std::vector<float>(
        {std::max(thresholds.at(i - 1), meanValues.at(i) - stdDevValues.at(i)),
         std::min(thresholds.at(i), meanValues.at(i) + stdDevValues.at(i))}));
    //        }
    //        else
    //        {

    //        }
  }
  thresholdRanges.push_back(std::vector<float>({thresholds.back(), grayMax}));

  // Test: Output (2)
  std::cout << "\n\nMin: ";
  std::cout << grayMin;
  std::cout << "\nMax: ";
  std::cout << grayMax;
  std::cout << "\n\nTheshold Ranges: ";
  for (std::vector<float> thresholdRange : thresholdRanges) {
    std::cout << "[";
    std::cout << thresholdRange.front();
    std::cout << ", ";
    std::cout << thresholdRange.back();
    std::cout << "], ";
  }
  std::cout << "\n";

  // Calculate gradients
  std::vector<std::future<void>> gradientFutures;
  gradientFutures.reserve(voxels.size<2>());
  for (size_t z = 0; z < voxels.size<2>(); z++) {
    gradientFutures.push_back(std::async([&]() {
      for (size_t y = 0; y < voxels.size<1>(); y++) {
        for (size_t x = 0; x < voxels.size<0>(); x++) {
          voxels(x, y, z).computeGradient(voxels, x, y, z);
        }
      }
    }));
  }
  std::cout << "Waiting for Gradients\n";
  for (auto& future : gradientFutures)
    future.wait();  // check if all gradients have been computed
  std::cout << "Gradients done!\n";
  gradientFutures.clear();
  gradientFutures.reserve(0);

  // Sort voxels in threshold ranges and isolate doubtful voxels
  std::vector<std::future<void>> thresholdRangeFutures;
  thresholdRangeFutures.reserve(voxels.size<2>());
  std::vector<std::future<void>> doubtfulNeighbourFutures;
  for (size_t z0 = 0; z0 < voxels.size<2>(); z0++) {
    thresholdRangeFutures.push_back(std::async(
        // TODO: This should probably not use & caputure
        [&](size_t z) {
          for (size_t y = 0; y < voxels.size<1>(); y++) {
            for (size_t x = 0; x < voxels.size<0>(); x++) {
              // label voxels according to the threshold range they fit in
              for (size_t i = 0; i < thresholdRanges.size(); i++) {
                if (i < thresholdRanges.size() - 1) {
                  if (thresholdRanges.at(i).front() <=
                          voxels(x, y, z).getGrayValue() &&
                      voxels(x, y, z).getGrayValue() <
                          thresholdRanges.at(i).back() &&
                      std::abs(voxels(x, y, z).getGradient()) < epsilon) {
                    voxels(x, y, z).addLabel(i);
                    break;  // at this point, voxels can only have one label
                  }
                } else  // last threshold range includes upper limit
                {
                  if (thresholdRanges.at(i).front() <=
                          voxels(x, y, z).getGrayValue() &&
                      voxels(x, y, z).getGrayValue() <=
                          thresholdRanges.at(i).back() &&
                      std::abs(voxels(x, y, z).getGradient()) < epsilon) {
                    voxels(x, y, z).addLabel(i);
                  }
                }
              }
            }
          }
        },
        z0));
  }
  std::cout << "Waiting for ThresholdRanges\n";
  for (auto& future : thresholdRangeFutures) future.wait();
  std::cout << "ThresholdRanges done!\n";
  thresholdRangeFutures.clear();
  thresholdRangeFutures.reserve(0);

  doubtfulNeighbourFutures.reserve(voxels.size<2>());
  for (size_t z0 = 0; z0 < voxels.size<2>(); z0++) {
    doubtfulNeighbourFutures.push_back(std::async(
        // TODO: This should probably not use & caputure
        [&](size_t z) {
          for (size_t y = 0; y < voxels.size<1>(); y++) {
            for (size_t x = 0; x < voxels.size<0>(); x++) {
              // Mark voxels as doubtful if their neighbours are not classified
              // as the same material as themselves; Checks for each neighbour,
              // if there actually is a neighbour, if it is labelles and if so,
              // if it has the same label as the current voxel
              if (!((!((x + 1) > voxels.size<0>()) &&
                     ((voxels(x, y, z).getFirstLabel() &&
                       voxels(x + 1, y, z).getFirstLabel()))) ||
                    (!((x - 1) > x) &&
                     ((voxels(x, y, z).getFirstLabel() &&
                       voxels(x - 1, y, z).getFirstLabel()))) ||
                    (!((y + 1) > voxels.size<1>()) &&
                     ((voxels(x, y, z).getFirstLabel() &&
                       voxels(x, y + 1, z).getFirstLabel()))) ||
                    (!((y - 1) > y) &&
                     ((voxels(x, y, z).getFirstLabel() &&
                       voxels(x, y - 1, z).getFirstLabel()))) ||
                    (!((z + 1) > voxels.size<2>()) &&
                     ((voxels(x, y, z).getFirstLabel() &&
                       voxels(x, y, z + 1).getFirstLabel()))) ||
                    (!((z - 1) > z) &&
                     ((voxels(x, y, z).getFirstLabel() &&
                       voxels(x, y, z - 1).getFirstLabel())))))
                voxels(x, y, z).clearLabels();
            }
          }
        },
        z0));
  }
  std::cout << "Waiting for Doubtful Neighbours\n";
  for (auto& future : doubtfulNeighbourFutures) future.wait();
  std::cout << "Doubtful Neighbours done!\n";
  doubtfulNeighbourFutures.clear();
  doubtfulNeighbourFutures.reserve(0);

  // Test: Output (3)
  //    size_t counter = 0;
  //    std::cout << "\n\nVoxel Classifications:\n";
  //    for (std::list<std::reference_wrapper<Voxel>> material :
  //    classifiedVoxels)
  //    {
  //        std::cout << "Material ";
  //        std::cout << counter;
  //        counter++;
  //        std::cout << ": ";
  //        for (Voxel voxel : material)
  //        {
  //            std::cout << voxel.getGrayValue();
  //            std::cout << ", ";
  //        }
  //        std::cout << "\n";
  //    }
  //    std::cout << "\nDoubtful Voxels: ";
  //    for (auto& doubtfulVoxel : doubtfulVoxels)
  //    {
  //        std::cout << doubtfulVoxel.first.get().getGrayValue();
  //        std::cout << ", ";
  //    }
  //    std::cout << "\n";

  // Label Propagation (overlapped)
  bool doubtChanged = true;
  size_t unlabelledVoxelCount = 1;  // any value other than 0 will work
  size_t doubtfulVoxelCount =
      1;  // any value other than 0 will work, doubtful in this case means "less
          // than 2 labels", meaning that like some once-labelled but correct
          // voxels are included

  for (size_t z = 0; z < voxels.size<2>(); z++) {
    for (size_t y = 0; y < voxels.size<1>(); y++) {
      for (size_t x = 0; x < voxels.size<0>(); x++) {
        if (voxels(x, y, z).getLabels().none()) {
          unlabelledVoxelCount++;
        } else {
          voxels(x, y, z).setTempLabels(
              voxels(x, y, z)
                  .getLabels());  // ensures, that voxels do not lose labels in
                                  // later step, where label changes are applied
        }
      }
    }
  }
  std::cout << unlabelledVoxelCount;

  // First step of label propagation. Propagate until doubtful voxels have at
  // least 1 label
  while (unlabelledVoxelCount > 0) {
    unlabelledVoxelCount = 0;
    std::vector<std::future<void>> propagationFutures;
    propagationFutures.reserve(voxels.size<2>());

    for (size_t z0 = 0; z0 < voxels.size<2>(); z0++) {
      propagationFutures.push_back(std::async(
          // TODO: This should probably not use & caputure
          [&](size_t z) {
            for (size_t y = 0; y < voxels.size<1>(); y++) {
              for (size_t x = 0; x < voxels.size<0>(); x++) {
                if (voxels(x, y, z).getLabelCount() < 1) {
                  voxels(x, y, z).setDoubt(true);

                  // Check labels of all direct neighbours and add to copy of
                  // voxel
                  Voxel tempVoxel = voxels(x, y, z);
                  calculateLabelChanges(tempVoxel, x + 1, y, z);
                  calculateLabelChanges(tempVoxel, x - 1, y, z);
                  calculateLabelChanges(tempVoxel, x, y + 1, z);
                  calculateLabelChanges(tempVoxel, x, y - 1, z);
                  calculateLabelChanges(tempVoxel, x, y, z + 1);
                  calculateLabelChanges(tempVoxel, x, y, z - 1);

                  // Check if labels changed, if so, stage changes
                  if (voxels(x, y, z).getLabels() != tempVoxel.getLabels()) {
                    doubtChanged = true;
                    voxels(x, y, z).setTempLabels(tempVoxel.getLabels());
                  }
                }
              }
            }
          },
          z0));
    }

    std::cout << "Waiting for Propagation\n";
    for (auto& future : propagationFutures) {
      future.wait();
    }

    propagationFutures.clear();
    propagationFutures.reserve(0);
    std::cout << "Propagation done!\n";

    // apply label changes and count remaining unlabelled voxels
    for (size_t z = 0; z < voxels.size<2>(); z++) {
      for (size_t y = 0; y < voxels.size<1>(); y++) {
        for (size_t x = 0; x < voxels.size<0>(); x++) {
          voxels(x, y, z).applyLabelChanges();
          if (voxels(x, y, z).getLabels().none()) unlabelledVoxelCount++;
        }
      }
    }
    std::cout << unlabelledVoxelCount;
    std::cout << " Unlabelled Voxels Left\n";
  }

  doubtChanged = true;  // resetting for while condition

  // Second step of label propagation. Propagate until doubtful voxels have at
  // least 2 labels
  while (doubtfulVoxelCount > 0 && doubtChanged) {
    doubtChanged = false;
    doubtfulVoxelCount = 0;
    std::vector<std::future<void>> propagationFutures;
    propagationFutures.reserve(voxels.size<2>());

    for (size_t z0 = 0; z0 < voxels.size<2>(); z0++) {
      // TODO: This should probably not use & caputure
      propagationFutures.push_back(std::async(
          [&](size_t z) {
            for (size_t y = 0; y < voxels.size<1>(); y++) {
              for (size_t x = 0; x < voxels.size<0>(); x++) {
                if ((voxels(x, y, z).getLabelCount() < 2) &&
                    (voxels(x, y, z).isDoubtful())) {
                  // Check labels of all direct neighbours and add to copy of
                  // voxel
                  Voxel tempVoxel = voxels(x, y, z);
                  calculateLabelChanges(tempVoxel, x + 1, y, z);
                  calculateLabelChanges(tempVoxel, x - 1, y, z);
                  calculateLabelChanges(tempVoxel, x, y + 1, z);
                  calculateLabelChanges(tempVoxel, x, y - 1, z);
                  calculateLabelChanges(tempVoxel, x, y, z + 1);
                  calculateLabelChanges(tempVoxel, x, y, z - 1);

                  // Check if labels changed. If so, stage changes
                  if (voxels(x, y, z).getLabels() != tempVoxel.getLabels()) {
                    doubtChanged = true;
                    voxels(x, y, z).setTempLabels(tempVoxel.getLabels());
                  }
                }
              }
            }
          },
          z0));
    }

    std::cout << "Waiting for Propagation\n";
    for (auto& future : propagationFutures) {
      future.wait();
    }
    propagationFutures.clear();
    propagationFutures.reserve(0);
    std::cout << "Propagation done!\n";

    // Count remaining doubtful voxels
    std::vector<std::future<size_t>> countingFutures;
    countingFutures.reserve(voxels.size<2>());
    for (size_t z = 0; z < voxels.size<2>(); z++) {
      for (size_t y = 0; y < voxels.size<1>(); y++) {
        for (size_t x = 0; x < voxels.size<0>(); x++) {
          voxels(x, y, z).applyLabelChanges();
          if (voxels(x, y, z).isDoubtful()) {
            if (voxels(x, y, z).getLabelCount() < 2) {
              doubtfulVoxelCount++;
            } else {
              voxels(x, y, z).setDoubt(false);
            }
          }
        }
      }
    }
    std::cout << doubtfulVoxelCount;
    std::cout << " Doubtful Voxels Left\n";
  }

  // Test: Output (4)
  std::cout << "\n\nAfter Propagation:\n";
  //    for (size_t z = 0; z < voxels.size<2>(); z++)
  //    {
  //        for (size_t y = 0; y < voxels.size<1>(); y++)
  //        {
  //            for (size_t x = 0; x < voxels.size<0>(); x++)
  //            {
  //                for (int var = 0; var < Voxel::MAX_LABELS; ++var) {
  //                    if (voxels(x, y, z).getLabels()[var]) {
  //                        std::cout << 1;
  //                    } else {
  //                        std::cout << 0;
  //                    }
  //                }
  //                std::cout << ";";
  //                std::cout << (int)voxels(x, y, z).getFirstLabel();
  //                std::cout << "\n";
  //            }
  //        }
  //    }

  // Adaptive Threshholding
  std::vector<std::future<void>> threshholdingFutures;
  threshholdingFutures.reserve(voxels.size<2>());
  for (size_t z0 = 0; z0 < voxels.size<2>(); z0++) {
    // TODO: This should probably not use & caputure
    threshholdingFutures.push_back(std::async(
        [&](size_t z) {
          for (size_t y = 0; y < voxels.size<1>(); y++) {
            for (size_t x = 0; x < voxels.size<0>(); x++) {
              int8_t actualLabel = adaptiveThresholding(voxels(x, y, z));
              //                    for (auto label : voxels(x, y,
              //                    z).getAllLabels()) {
              //                        std::cout << (int)label;
              //                        std::cout << ",";
              //                    }
              /*                    std::cout << "|"*/;
              voxels(x, y, z).clearLabels();
              voxels(x, y, z).addLabel(actualLabel);
              //                    if (actualLabel != -1)
              //                        std::cout << ":O";
              //                    for (auto label : voxels(x, y,
              //                    z).getAllLabels()) {
              //                        std::cout << (int)label;
              //                        std::cout << ",";
              //                    }
              /*                    std::cout << "\n"*/;
            }
          }
        },
        z0));
  }
  for (auto& future : threshholdingFutures) future.wait();
  threshholdingFutures.clear();
  threshholdingFutures.reserve(0);

  //    for (auto &material : classifiedVoxels) material.clear();
  //    for (size_t z = 0; z < voxels.size<2>(); z++)
  //    {
  //        for (size_t y = 0; y < voxels.size<1>(); y++)
  //        {
  //            for (size_t x = 0; x < voxels.size<0>(); x++)
  //            {
  //                if (voxels(x, y, z).getLabels().any())
  //                    for (size_t i = 0; i < thresholdRanges.size(); i++)
  //                        for (size_t label : voxels(x, y, z).getLabels()) if
  //                        (i == label)
  //                        classifiedVoxels.at(i).push_back(voxels(x, y, z));
  //            }
  //        }
  //    }
  std::cout << "AdaptiveThresholding done!\n";

  // Test: Output (5)
  //    std::cout << "\n\nAfter adaptive Threshholding:\n";
  //    counter = 0;
  //    for (std::list<std::reference_wrapper<Voxel>> material :
  //    classifiedVoxels)
  //    {
  //        std::cout << "Material ";
  //        std::cout << counter;
  //        counter++;
  //        std::cout << ": ";
  //        for (Voxel voxel : material)
  //        {
  //            std::cout << voxel.getGrayValue();
  //            std::cout << ", ";
  //        }
  //        std::cout << "\n";
  //    }

  size_t unlabelled = 0;
  size_t duallabelled = 0;
  for (size_t z = 0; z < voxels.size<2>(); z++) {
    for (size_t y = 0; y < voxels.size<1>(); y++) {
      for (size_t x = 0; x < voxels.size<0>(); x++) {
        if (voxels(x, y, z).getLabelCount() != 1) {
          std::cout << voxels(x, y, z).getGrayValue();
        }
        if (voxels(x, y, z).getLabels().none()) {
          std::cout << " NONE!";
          std::cout << ", ";
          unlabelled++;
        }
        if (voxels(x, y, z).getLabelCount() > 1) {
          std::cout << " TOO MANY!";
          std::cout << ", ";
          duallabelled++;
        }
      }
    }
  }

  //    for (size_t z = 0; z < voxels.size<2>(); z++)
  //    {
  //        for (size_t y = 0; y < voxels.size<1>(); y++)
  //        {
  //            for (size_t x = 0; x < voxels.size<0>(); x++)
  //            {
  //                std::cout << x;
  //                std::cout << ";";
  //                std::cout << y;
  //                std::cout << ";";
  //                std::cout << z;
  //                std::cout << ";";
  //                std::cout << (int)voxels(x, y, z).getFirstLabel();
  //                std::cout << ";";
  //                std::cout << (int)voxels(x, y, z).getLabelCount();
  //                std::cout << ";";
  //                std::cout << voxels(x, y, z).getGrayValue();
  //                std::cout << ";";
  //                std::cout << voxels(x, y, z).getGradient();
  //                std::cout << "\n";
  //            }
  //        }
  //    }

  std::cout << "\nDoubtful Voxels: ";
  std::cout << "\n";
  std::cout << "Unlabelled: ";
  std::cout << unlabelled;
  std::cout << "\n";
  std::cout << "Multiple Labelled: ";
  std::cout << duallabelled;
  std::cout << "\n";
}
