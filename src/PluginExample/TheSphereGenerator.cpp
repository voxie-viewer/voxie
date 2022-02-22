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

#include "TheSphereGenerator.hpp"

#include <Voxie/Data/VolumeNode.hpp>

#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

#include <VoxieClient/Exception.hpp>

#include <PluginExample/Prototypes.hpp>

#include <algorithm>

#include <math.h>

using namespace vx;
using namespace vx::io;

using namespace ::internal;

TheSphereGenerator::TheSphereGenerator()
    : FilterNode(getPrototypeSingleton()),
      properties(new TheSphereGeneratorProperties(this)) {
  setAutomaticDisplayName("The Sphere Generator");
}

TheSphereGenerator::~TheSphereGenerator() {}

QSharedPointer<vx::VolumeDataVoxel> TheSphereGenerator::genSphereImpl(
    int size, quint32 seed) {
  // QVector3D spacing(0.7f, 0.7f, 0.7f);
  QVector3D spacing(1e-3f, 1e-3f, 1e-3f);

  QVector3D origin(-size / 2.0f, -size / 2.0f, -size / 2.0f);
  origin *= spacing;

  auto data = VolumeDataVoxel::createVolume(size, size, size);
  data->setOrigin(origin);
  // TODO: Get rid of srand()/rand() (because of multi-threading etc.)
  srand(seed);  // Seed some random data

  data->performInGenericContext([&](auto& data2) {
    typedef
        typename std::remove_reference<decltype(data2)>::type::VoxelType Type;

    data2.transformCoordinateSingledThreaded([size](size_t x, size_t y,
                                                    size_t z, Type) -> Type {
      float px = static_cast<float>(x) / size * 2.0f - 1.0f;
      float py = static_cast<float>(y) / size * 2.0f - 1.0f;
      float pz = static_cast<float>(z) / size * 2.0f - 1.0f;

      float voxel = std::max(1.0f - sqrtf(px * px + py * py + pz * pz),
                             0.0f);  // Get distance
      // qDebug() << x << y << z << px << py << pz << voxel;
      voxel = fmax(voxel - 0.1f, 0.0f);  // 0.2 Thick radius
      voxel = powf(voxel, 1.8f);         // Pretty strong falloff

      // Make some noise!
      voxel +=
          0.04f *
          (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 0.5f);

      return static_cast<Type>(voxel);
    });

    /*
    auto range = data2.getMinMaxValue();
    data2.transform([range](Type v) -> Type {
      return (v - range.first) / (range.second - range.first);
    });
    */

    data2.setSpacing(spacing);
  });
  return data;
}

QSharedPointer<vx::io::RunFilterOperation> TheSphereGenerator::calculate() {
  QSharedPointer<vx::io::RunFilterOperation> operation =
      RunFilterOperation::createRunFilterOperation();
  auto data = genSphereImpl(this->properties->size(), this->properties->seed());

  auto output = dynamic_cast<VolumeNode*>(this->properties->output());
  if (!output) {
    output = dynamic_cast<VolumeNode*>(
        VolumeNode::getPrototypeSingleton()
            ->create(QMap<QString, QVariant>(), QList<Node*>(),
                     QMap<QString, QDBusVariant>())
            .data());
    if (!output) {
      qWarning() << "Failed to create VolumeNode";
      return operation;
    }
    output->setAutomaticDisplayName("Sphere");
    this->properties->setOutput(output);
  }

  output->setData(data);
  return operation;
}

NODE_PROTOTYPE_IMPL(TheSphereGenerator)
