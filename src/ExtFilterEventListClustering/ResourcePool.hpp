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

#include <QSharedPointer>
#include <QVector>

#include <functional>

namespace vx {

template <typename T>
class ResourcePool {
 public:
  using Ptr = QSharedPointer<T>;
  using Comparator = std::function<bool(const T&, const T&)>;

  ResourcePool(Comparator comparator = nullptr) {
    if (comparator) {
      ptrComparator = [comparator](const Ptr& a, const Ptr& b) {
        return comparator(*a, *b);
      };
    }
  }

  template <typename Func>
  Ptr find(Func predicate) {
    for (const Ptr& entry : resources) {
      if (predicate(entry)) {
        return entry;
      }
    }
    return Ptr();
  }

  template <typename Func>
  Ptr extract(Func predicate) {
    for (auto it = resources.begin(); it != resources.end(); ++it) {
      if (predicate(**it)) {
        auto entry = *it;
        resources.erase(it);
        return entry;
      }
    }
    return Ptr();
  }

  void add(Ptr resource) {
    if (ptrComparator) {
      resources.insert(std::lower_bound(resources.begin(), resources.end(),
                                        resource, ptrComparator),
                       resource);
    } else {
      resources.append(resource);
    }
  }

 private:
  using PtrComparator = std::function<bool(const Ptr&, const Ptr&)>;

  PtrComparator ptrComparator;
  QVector<Ptr> resources;
};

}  // namespace vx
