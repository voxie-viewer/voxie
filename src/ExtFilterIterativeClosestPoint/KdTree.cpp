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

#include "KdTree.hpp"
#include <QVector3D>
#include <cmath>

template <class T>
KdTree<T>::KdTree(std::vector<T>& pointCloud) : pcRef_(pointCloud), depth_(0) {
  if (pointCloud.size() == 0) {
    throw "size of std::vector is zero";
  }
  tmpArray = new uint32_t[pointCloud.size()];
  indexArray = new uint32_t[pointCloud.size()];
  for (uint32_t i = 0; i < pointCloud.size(); i++) {
    indexArray[i] = i;
  }
  build(pointCloud, 0, pointCloud.size());

  delete[] tmpArray;
  delete[] indexArray;
}

template <class T>
KdTree<T>::KdTree(uint32_t* rootTmpArray, uint32_t* rootIndexArray,
                  std::vector<T>& pointCloud, int start, int end, int depth)
    : pcRef_(pointCloud), depth_(depth) {
  tmpArray = rootTmpArray;
  indexArray = rootIndexArray;
  build(pointCloud, start, end);
}

template <class T>
KdTree<T>::~KdTree() {
  for (int i = 0; i < 2; i++) {
    if (children_[i] != nullptr) {
      delete children_[i];
    }
  }
}

template <class T>
void KdTree<T>::build(std::vector<T>& pointCloud, int start, int end) {
  index_ = 0;

  sortOn_ = depth_ % dim();
  start_ = start;
  end_ = end;

  if (end - start == 1) {
    index_ = indexArray[start];
    return;
  }

  mergeSort(pointCloud, start, end);

  int mid = (end + start) / 2;

  index_ = indexArray[mid];

  /*
   *     mid
   *    /   \
   * left  right
   *
   * mid point is already stored
   */

  int numPointsHigh = (end - mid) - 1;
  int numPointsLow = (mid - start);

  if (numPointsHigh > 0) {
    children_[1] =
        new KdTree(tmpArray, indexArray, pointCloud, mid + 1, end, depth_ + 1);
  }

  if (numPointsLow > 0) {
    children_[0] =
        new KdTree(tmpArray, indexArray, pointCloud, start, mid, depth_ + 1);
  }
}

template <class T>
std::vector<uint32_t> KdTree<T>::radiusSearchIndex(T& target, float radius) {
  std::vector<uint32_t> results;
  radiusSearch(target, radius, results);
  return results;
}

template <class T>
std::vector<T> KdTree<T>::radiusSearch(T& target, float radius) {
  std::vector<T> results;
  std::vector<uint32_t> indices;
  radiusSearch(target, radius, indices);
  for (uint32_t i = 0; i < indices.size(); i++) {
    results.push_back(pcRef_[indices[i]]);
  }
  return results;
}

// private
template <class T>
void KdTree<T>::radiusSearch(T& target, float radius,
                             std::vector<uint32_t>& results) {
  if (distSquared(pcRef_[index_], target) < radius * radius) {
    results.push_back(index_);
  }
  float minCoord = target[sortOn_] - radius;
  float maxCoord = target[sortOn_] + radius;

  // search left side of hyperplane
  if (minCoord < splitValue()) {
    if (children_[0] != nullptr) {
      children_[0]->radiusSearch(target, radius, results);
    }
  }
  // search right side of hyperplane
  if (maxCoord > splitValue()) {
    if (children_[1] != nullptr) {
      children_[1]->radiusSearch(target, radius, results);
    }
  }
}

template <class T>
uint32_t KdTree<T>::nearestNeighborIndex(T& target) {
  return nearestNeighborIndex(target, -1);
}

template <class T>
bool KdTree<T>::nearestNeighborIndex(T& target, float radius, int& index) {
  index = nearestNeighborIndex(target, radius);
  T* result = &(pcRef_[index]);
  if (distSquared(*result, target) > radius * radius && radius >= 0) {
    return false;
  }
  return true;
}

// private
template <class T>
uint32_t KdTree<T>::nearestNeighborIndex(T& target, float radius) {
  KdTree* nextNode;
  KdTree* otherNode;
  float dist = target[sortOn_] - splitValue();
  if (dist < 0) {
    // left side of hyperplane first
    nextNode = children_[0];
    otherNode = children_[1];
  } else {
    // right side of hyperplane first
    otherNode = children_[0];
    nextNode = children_[1];
  }

  // use radius for search?
  if (radius >= 0) {
    // target and NN is left of hyperplane
    if (dist + radius < 0) {
      otherNode = nullptr;
    }
    // target and NN is right of hyperplane
    if (dist - radius > 0) {
      otherNode = nullptr;
    }
  }

  uint32_t tmp;
  uint32_t best = index_;
  if (nextNode != nullptr) {
    tmp = nextNode->nearestNeighborIndex(target, radius);
    best = bestMatch(target, tmp, best);
  }

  if (otherNode != nullptr) {
    // sphere intersects the hyperplane, potential match on the other side
    float radius2 = std::sqrt(distSquared(target, pcRef_[best]));
    if (radius2 >= std::abs(dist)) {
      tmp = otherNode->nearestNeighborIndex(target, radius2);
      best = bestMatch(target, tmp, best);
    }
  }

  return best;
}

template <class T>
T* KdTree<T>::nearestNeighbor(T& target) {
  return nearestNeighbor(target, -1);
}

template <class T>
T* KdTree<T>::nearestNeighbor(T& target, float radius) {
  T* result = &(pcRef_[nearestNeighborIndex(target, radius)]);
  // if nearestNeighbor got called with radius < 0 the result is ok
  // else result doesn't mean anything if distance is greater than radius (not
  // even NN)
  if (distSquared(*result, target) > radius * radius && radius >= 0) {
    result = nullptr;
  }
  return result;
}

template <class T>
/**
 * @brief Should not be used because inefficently implemented.
 * No useful std container, thus the result gets sorted each step. Also needs
 * refactoring.
 * @param target
 * @param k
 * @return
 */
std::vector<std::pair<float, uint32_t>> KdTree<T>::kNNIndex(const T& target,
                                                            uint32_t k) {
  std::vector<std::pair<float, uint32_t>> kNN;
  kNNIndex(target, k, kNN);
  return kNN;
}

// private
template <class T>
uint32_t KdTree<T>::kNNIndex(const T& target, uint32_t k,
                             std::vector<std::pair<float, uint32_t>>& kNN) {
  KdTree* nextNode;
  KdTree* otherNode;
  float dist = target[sortOn_] - splitValue();
  if (dist < 0) {
    // left side of hyperplane first
    nextNode = children_[0];
    otherNode = children_[1];
  } else {
    // right side of hyperplane first
    otherNode = children_[0];
    nextNode = children_[1];
  }

  uint32_t tmp;
  uint32_t best = index_;
  if (nextNode != nullptr) {
    tmp = nextNode->kNNIndex(target, k, kNN);
    best = bestMatch(target, tmp, best);
  }

  if (otherNode != nullptr) {
    // sphere intersects the hyperplane, potential match on the other side
    float radius = std::sqrt(distSquared(target, pcRef_[best]));
    if (radius >= std::abs(dist)) {
      tmp = otherNode->kNNIndex(target, k, kNN);
      best = bestMatch(target, tmp, best);
    }
  }

  // insert found node
  kNN.push_back(std::make_pair(distSquared(target, pcRef_[index_]), index_));

  // keep size at k
  if (kNN.size() > k) {
    sort(kNN.begin(), kNN.end());
    kNN.pop_back();
  }

  return kNN.back().second;
}

template <class T>
uint32_t KdTree<T>::bestMatch(const T& target, uint32_t p, uint32_t q) {
  if (distSquared(target, pcRef_[p]) < distSquared(target, pcRef_[q])) {
    return p;
  } else {
    return q;
  }
}

template <class T>
void KdTree<T>::mergeSort(std::vector<T>& pointCloud, int start, int end) {
  if (end - start > 1) {
    int mid = (end + start) / 2;
    mergeSort(pointCloud, start, mid);
    mergeSort(pointCloud, mid, end);

    merge(pointCloud, start, mid, end);
  }
}

template <class T>
void KdTree<T>::merge(std::vector<T>& pointCloud, int left, int mid,
                      int right) {
  int leftEnd = mid;
  int tempPos = left;

  // indirect merge sort, sort indices instead of pointCloud directly

  // copy to tmp
  for (int i = left; i < right; i++) {
    tmpArray[i] = indexArray[i];
  }

  // compare and remove
  while (left < leftEnd && mid < right) {
    uint32_t iLeft = tmpArray[left];
    uint32_t iMid = tmpArray[mid];
    if (pointCloud[iLeft][sortOn_] <= pointCloud[iMid][sortOn_]) {
      indexArray[tempPos] = tmpArray[left];
      tempPos++;
      left++;
    } else {
      indexArray[tempPos] = tmpArray[mid];
      tempPos++;
      mid++;
    }
  }

  // copy rest of left
  while (left < leftEnd) {
    indexArray[tempPos] = tmpArray[left];
    left++;
    tempPos++;
  }

  // copy rest of right
  while (mid < right) {
    indexArray[tempPos] = tmpArray[mid];
    mid++;
    tempPos++;
  }
}

template <class T>
float KdTree<T>::distSquared(const T& point1, const T& point2) {
  float len = 0;
  for (uint32_t i = 0; i < dim(); i++) {
    len += (point1[i] - point2[i]) * (point1[i] - point2[i]);
  }
  return len;
}

template <>
uint32_t KdTree<QVector3D>::dim() {
  return 3;
}

template <>
uint32_t KdTree<std::vector<float>>::dim() {
  return pcRef_[0].size();
}

template class KdTree<std::vector<float>>;
template class KdTree<QVector3D>;
