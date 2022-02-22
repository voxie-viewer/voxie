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

#pragma once

#include <QVector3D>

// TODO refactoring
// combine KNN with nearestNeighbor
// make KNN efficient
// make target const
// use always bool as return type instead of uint32_t?
template <class T>
class KdTree {
 public:
  KdTree(std::vector<T>& pointCloud);
  virtual ~KdTree();

  T* nearestNeighbor(T& target);
  T* nearestNeighbor(T& target, float radius);
  uint32_t nearestNeighborIndex(T& target);
  bool nearestNeighborIndex(T& target, float radius, int& index);
  std::vector<T> radiusSearch(T& target, float searchRadius);
  std::vector<uint32_t> radiusSearchIndex(T& target, float searchRadius);
  std::vector<std::pair<float, uint32_t>> kNNIndex(const T& target, uint32_t k);
  std::vector<T>& getPcRef() { return pcRef_; }

 private:
  KdTree(uint32_t* rootTmpArray, uint32_t* rootIndexArray,
         std::vector<T>& pointCloud, int start, int end, int depth);
  void build(std::vector<T>& pointCloud, int start, int end);

  std::vector<T>& pcRef_;
  KdTree* children_[2] = {nullptr, nullptr};
  uint32_t index_;
  uint32_t* tmpArray;
  uint32_t* indexArray;
  int sortOn_;
  int start_;
  int end_;
  int depth_;

  uint32_t nearestNeighborIndex(T& target, float radius);
  void radiusSearch(T& target, float searchRadius,
                    std::vector<uint32_t>& results);
  uint32_t kNNIndex(const T& target, uint32_t k,
                    std::vector<std::pair<float, uint32_t>>& kNN);
  void mergeSort(std::vector<T>& pointCloud, int start, int end);
  void merge(std::vector<T>& pointCloud, int left, int mid, int right);
  uint32_t bestMatch(const T& target, uint32_t p, uint32_t q);
  float splitValue() { return pcRef_[index_][sortOn_]; }
  float distSquared(const T& point1, const T& point2);
  uint32_t dim();
};
