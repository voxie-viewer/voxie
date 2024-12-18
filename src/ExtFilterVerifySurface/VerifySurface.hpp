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

#ifndef VERIFYSURFACE_H
#define VERIFYSURFACE_H

#include <VoxieClient/Array.hpp>
#include <VoxieClient/ClaimedOperation.hpp>
#include <VoxieClient/Format.hpp>

class VerifySurface {
 public:
  VerifySurface(vx::Array2<const uint32_t> triangles_nominal,
                vx::Array2<const float> vertices_nominal, QString* report);

  template <typename... T>
  void fail(const char* str, const T&... par) {
    QString msg = vx::format(str, par...);
    if (report)
      *report += "- " + msg + "\n";
    else
      throw vx::Exception("de.uni_stuttgart.Voxie.Error", msg);
  }

  void run(vx::ClaimedOperation<
           de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& op);

  std::vector<std::tuple<std::uint32_t, std::uint32_t, std::int32_t>>
      failedEdges;

 private:
  vx::Array2<const uint32_t> triangles;
  vx::Array2<const float> vertices;
  QString* report;
};

#endif  // VERIFYSURFACE_H
