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

#include "Plane.hpp"

#include <PluginVis3D/Prototypes.hpp>

#include <PluginVis3D/Data/PlaneData.hpp>

#include <PluginVis3D/Helper/PlaneShader.hpp>

#include <Voxie/Data/Color.hpp>
#include <Voxie/Node/NodePrototype.hpp>

#include <Voxie/PropertyObjects/PlaneNode.hpp>

#include <Voxie/Vis/OpenGLWidget.hpp>

#include <QtCore/QTimer>

#include <QtOpenGL/QGLWidget>  // TODO: remove

using namespace vx;
using namespace vx::visualization;
using namespace vx::vis3d;

// TODO: Move most stuff (including slice computation) out of
// PlanePerShareGroup/PlaneData into Plane class, keep only stuff which really
// needs the OpenGL context into PlanePerShareGroup

// TODO: Changing the slice resolution seems to move the image

// TODO: Support moving planes via mouse?
/*
  // Move plane instead of surface:
  if (movePlaneButton->isChecked()) {
    auto plane = view->selectedPlane();
    if (!plane) return;
    auto origin = plane->plane()->origin + offset;
    plane->setOrigin(origin);
    return;
  }

  // Rotate plane instead of surface:
  if (movePlaneButton->isChecked()) {
    auto plane = view->selectedPlane();
    if (!plane) return;
    plane->setRotation(rotation * plane->plane()->rotation);
    return;
  }
*/

// TODO: Make sure that rendering for screenshot triggers
// PlanePerShareGroup::planeTextureTimeoutElapsed when needed

namespace {
class PlanePerShareGroup : public Object3DPerShareGroup {
 public:
  const int LOW_RESOLUTION = 128;
  const int PLANE_CHANGE_TIMEOUT = 2000;

  QSharedPointer<Object3DRenderContextPerShareGroup> shareContext;

  // QPointer<Plane> plane;

  QOpenGLFunctions functions;

  // OpenGLDrawUtils utils;

  PlaneShader planeShader;

  QSharedPointer<PlaneData> planeData;

  QPointer<vx::vis3d::Plane> planeObj;

  QTimer textureTimer;

  PlanePerShareGroup(
      const QSharedPointer<Object3DRenderContextPerShareGroup>& shareContext)
      : shareContext(shareContext) {
    textureTimer.setSingleShot(true);
    connect(&textureTimer, &QTimer::timeout, this,
            &PlanePerShareGroup::planeTextureTimeoutElapsed);
  }

  // Must be called with OpenGL context current
  void setPlaneData(vx::vis3d::Plane* planeObj,
                    const QSharedPointer<Colorizer>& colorizer,
                    vx::PlaneNode* plane,
                    object3d_prop::PlaneProperties* properties) {
    this->planeObj = planeObj;

    QSharedPointer<QOpenGLBuffer> vertexBuffer(new QOpenGLBuffer());
    if (!vertexBuffer->create())
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "Plane vertex buffer create failed");

    // TODO: This is a hack, what kind of pointer should be used here?
    auto planeShared = QSharedPointer<vx::PlaneNode>(
        plane, [](vx::PlaneNode* ptr) { Q_UNUSED(ptr); });

    planeData =
        makeSharedQObject<PlaneData>(planeShared, vertexBuffer, colorizer);

    connect(planeObj, &vx::vis3d::Plane::regenerateSliceImage, this,
            &PlanePerShareGroup::requestLoadSliceImage);

    connect(plane, &PlaneNode::rotationChanged, this,
            [this](QQuaternion rotation) {
              (void)rotation;
              qDebug() << "rotChanged";
              this->requestLoadSliceImage();
            });
    connect(plane, &PlaneNode::originChanged, this, [this](QVector3D origin) {
      (void)origin;
      qDebug() << "origChanged";
      this->requestLoadSliceImage();
    });

    auto planeDataPtr =
        makeSharedQObject<PlaneData>(planeShared, vertexBuffer, colorizer);

    connect(planeDataPtr.data(), &PlaneData::drawTextureFlagChanged, this,
            [this] {
              shareContext->select();
              if (!planeData->drawTexture()) {
                shareContext->unselect();
                return;
              }
              if (textureTimer.isActive()) textureTimer.stop();
              loadSliceImage(planeData->resolution());
              shareContext->select();
              auto slice = planeData->getSlice();
              auto bbox = slice ? slice->getBoundingRectangle()
                                : planeData->getBoundingRectangle();  // TODO
              planeShader.updateBuffers(planeData, bbox);
              // TODO: update();
              shareContext->unselect();
            });

    // TODO
    shareContext->select();
    auto slice = planeData->getSlice();
    auto bbox = slice ? slice->getBoundingRectangle()
                      : planeData->getBoundingRectangle();  // TODO
    planeShader.updateBuffers(planeDataPtr, bbox);

    planeData = planeDataPtr;

    QObject::connect(properties,
                     &object3d_prop::PlaneProperties::showVolumeSliceChanged,
                     this, [this, properties](bool value) {
                       planeData->setDrawTexture(
                           value && properties->sliceVolume() != nullptr);
                       if (this->planeObj) this->planeObj->triggerRendering();
                     });

    connect(properties, &object3d_prop::PlaneProperties::sliceVolumeChanged,
            this,
            // TODO: capture plane here?
            [this, plane, properties](vx::Node* value) {
              // TODO
              shareContext->select();

              qDebug() << "volumeChanged to" << value;

              if (!value) {
                planeData->setDrawTexture(false);
                planeData->removeSlice();
                if (this->planeObj) this->planeObj->triggerRendering();
              } else {
                auto volumeNode = dynamic_cast<VolumeNode*>(value);
                if (value && !volumeNode) {
                  qWarning() << "Could not cast Node to VolumeNode";
                  return;
                }
                // TODO
                // this->_colorizerWidget->setVolumeNode(volumeNode);

                planeData->setDrawTexture(properties->showVolumeSlice());

                auto slice2 = QSharedPointer<Slice>(new Slice(volumeNode));

                slice2->setOrigin(plane->plane()->origin);
                connect(plane, &PlaneNode::originChanged,
                        [=] { slice2->setOrigin(plane->plane()->origin); });
                connect(volumeNode, &VolumeNode::adjustedPositionChanged, [=] {
                  slice2->setOrigin(plane->plane()->origin);
                  requestLoadSliceImage();
                });
                slice2->setRotation(plane->plane()->rotation);
                connect(plane, &PlaneNode::rotationChanged,
                        [=] { slice2->setRotation(plane->plane()->rotation); });
                connect(volumeNode, &VolumeNode::adjustedRotationChanged, [=] {
                  slice2->setRotation(plane->plane()->rotation);
                  requestLoadSliceImage();
                });
                planeData->setSlice(slice2);
                requestLoadSliceImage();
              }
            });

    // TODO: hack
    properties->sliceVolumeChanged(properties->sliceVolume());

    connect(properties,
            &object3d_prop::PlaneProperties::sliceTextureResolutionChanged,
            this, [this](const QString& value) {
              if (!value.startsWith("de.uni_stuttgart.Voxie.Object3D.Plane."
                                    "SliceTextureResolution.R")) {
                qWarning() << "Unknown TextureResolution value:" << value;
                return;
              }
              int res = value
                            .mid(strlen("de.uni_stuttgart.Voxie.Object3D."
                                        "Plane.SliceTextureResolution.R"))
                            .toInt();
              planeData->setSliceResolution(res);
              loadSliceImage(res);
            });
    // TODO: hack
    properties->sliceTextureResolutionChanged(
        properties->sliceTextureResolution());

    // TODO
    // if (planeData->drawTexture()) loadSliceImage();
    // requestLoadSliceImage();
  }

  void planeTextureTimeoutElapsed() {
    qDebug() << "Load full texture";

    loadSliceImage(planeData->resolution());
  }

  void requestLoadSliceImage() {
    shareContext->select();
    qDebug() << "requestLoadSliceImage(): planeData->drawTexture()"
             << planeData->drawTexture();
    if (planeData->drawTexture()) {
      if (textureTimer.isActive()) textureTimer.stop();
      if (planeData->resolution() != LOW_RESOLUTION)
        textureTimer.start(PLANE_CHANGE_TIMEOUT);
      loadSliceImage(LOW_RESOLUTION);
    }
  }

  void loadSliceImage(int resolution) {
    shareContext->select();

    qDebug() << "LSI";

    auto slice = planeData->getSlice();
    // auto bbox = planeData->getBoundingRectangle();
    // TODO: actually make the slice this large
    auto bbox = slice ? slice->getBoundingRectangle()
                      : planeData->getBoundingRectangle();  // TODO
    QSharedPointer<Texture> texture;

    if (!slice.isNull()) {
      auto sliceImage =
          slice->generateImage(bbox, QSize(resolution, resolution));
      // TODO: Don't use QGLWidget here
      /*
      qDebug() << "toQImage" << planeData->colorizer().data();
      for (const auto& entry : planeData->colorizer()->getEntries())
        qDebug() << "V" << entry.value();
      */
      auto img = QGLWidget::convertToGLFormat(
          planeData->colorizer()->toQImage(sliceImage));

      auto tex = planeData->getTexture();

      GLuint textureID;
      if (!tex) {
        functions.glGenTextures(1, &textureID);
        qDebug() << "Created Slice Texture with ID: " << textureID;
      } else
        textureID = tex->textureID();

      texture = QSharedPointer<Texture>(
          new Texture(textureID, img.constBits(), resolution, resolution,
                      resolution * resolution * sizeof(unsigned int), true));
      connect(texture.data(), &QObject::destroyed, [this, texture] {
        const GLuint id = texture->textureID();
        functions.glDeleteTextures(1, &id);
      });

      shareContext->select();
      auto slice2 = planeData->getSlice();
      auto bbox2 = slice2 ? slice2->getBoundingRectangle()
                          : planeData->getBoundingRectangle();  // TODO
      planeShader.updateBuffers(planeData, bbox2);
      // shareContext->unselect();
      // TODO
      // update();
    }
    planeData->setTexture(texture);

    qDebug() << "LSI done";

    if (planeObj) planeObj->triggerRendering();

    shareContext->unselect();
  }
};
}  // namespace

vx::vis3d::Plane::Plane()
    : Object3DNode(getPrototypeSingleton()),
      properties(new object3d_prop::PlaneProperties(this)) {
  properties->setColor(PlaneData::nextColor());

  // Rerender the scene when the plane color changes
  QObject::connect(properties, &object3d_prop::PlaneProperties::colorChanged,
                   this, &Plane::triggerRendering);

  // Update bounding box when volume changes
  QObject::connect(properties,
                   &object3d_prop::PlaneProperties::sliceVolumeChanged, this,
                   &Plane::boundingBoxChanged);
  // TODO: update bounding box when either the volume
  // adjustedRotation/adjustedPosition changes or the volume size changes
  // because a new VolumeData is set

  QObject::connect(properties, &object3d_prop::PlaneProperties::planeChanged,
                   this, [this](Node* obj) {
                     auto prop = dynamic_cast<vx::PlaneNode*>(obj);

                     if (conn1) {
                       QObject::disconnect(conn1);
                       conn1 = QMetaObject::Connection();
                     }
                     if (conn2) {
                       QObject::disconnect(conn2);
                       conn2 = QMetaObject::Connection();
                     }

                     if (prop) {
                       conn1 = connect(prop, &PlaneNode::rotationChanged, this,
                                       &Plane::planeOrigOrientChanged);
                       conn2 = connect(prop, &PlaneNode::originChanged, this,
                                       &Plane::planeOrigOrientChanged);
                     }
                     Q_EMIT planeOrigOrientChanged();
                   });

  // Rerender when the plane changes
  QObject::connect(this, &Plane::planeOrigOrientChanged, this,
                   &Plane::triggerRendering);

  // Update clipping when either the clipping direction or the plane changes
  QObject::connect(properties,
                   &object3d_prop::PlaneProperties::clippingDirectionChanged,
                   this, &Plane::clippingPlanesChanged);
  QObject::connect(this, &Plane::planeOrigOrientChanged, this,
                   &Plane::clippingPlanesChanged);

  colorizer = makeSharedQObject<Colorizer>();

  QObject::connect(
      properties,
      &object3d_prop::PlaneProperties::sliceValueColorMappingChanged, this,
      [this](const QList<ColorizerEntry>& entries) {
        this->colorizer->setEntries(entries);
        Q_EMIT this->regenerateSliceImage();
        /*
        qDebug() << "valueColorMappingChanged" << colorizer;
        for (const auto& entry : this->colorizer->getEntries())
          qDebug() << "V" << entry.value();
        */
      });
  this->colorizer->setEntries(properties->sliceValueColorMapping());
  Q_EMIT this->regenerateSliceImage();
}
vx::vis3d::Plane::~Plane() {}

QSharedPointer<Object3DPerShareGroup> vx::vis3d::Plane::newPerShareGroup(
    const QSharedPointer<Object3DRenderContextPerShareGroup>& shareContext) {
  // qDebug() << "Plane::newPerShareGroup()";
  QSharedPointer<PlanePerShareGroup> ptr(
      new PlanePerShareGroup(shareContext),
      [](QObject* obj) { obj->deleteLater(); });
  ptr->functions.initializeOpenGLFunctions();

  // ptr->utils.initialize();

  QString res = ptr->planeShader.initialize();
  if (res != "")
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    "Failed to load plane shaders: " + res);

  QObject::connect(properties, &object3d_prop::PlaneProperties::planeChanged,
                   ptr.data(), [obj = ptr.data()](Node* planeObj) {
                     Q_UNUSED(planeObj);
                     obj->planeData.reset();
                   });

  return ptr;
}

void vx::vis3d::Plane::render(
    const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
    const QSharedPointer<Object3DPerContext>& perContext,
    Object3DRenderContext& context) {
  Q_UNUSED(perShareGroup);
  Q_UNUSED(perContext);
  Q_UNUSED(context);
  // everything is done in renderTransparent()
}

// TODO: Make this safe for multithreading?
void vx::vis3d::Plane::renderTransparent(
    const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
    const QSharedPointer<Object3DPerContext>& perContext,
    Object3DRenderContext& context) {
  Q_UNUSED(perContext);
  // qDebug() << "Render Plane";

  auto prop = dynamic_cast<vx::PlaneNode*>(this->properties->plane());
  if (!prop)  // No plane connected => Nothing to do
    return;

  auto psg = qSharedPointerDynamicCast<PlanePerShareGroup>(perShareGroup);
  if (!psg)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a PlanePerShareGroup");

  if (!psg->planeData) psg->setPlaneData(this, colorizer, prop, properties);

  QMatrix4x4 matViewProj = context.projectionMatrix() * context.viewMatrix();

  psg->functions.glEnable(GL_BLEND);
  psg->functions.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  psg->functions.glDepthMask(GL_FALSE);

  matViewProj.translate(prop->plane()->origin);
  matViewProj.rotate(prop->plane()->rotation);

  auto color = properties->color().asQVector4D();
  if (context.highlightedNode() == this) color.setW(0.8f);
  // qDebug() << "Calling psg->planeShader.draw() with" << psg->planeData <<
  // matViewProj << color;
  psg->planeShader.draw(psg->planeData, matViewProj, color);

  psg->functions.glDepthMask(GL_TRUE);
  psg->functions.glDisable(GL_BLEND);
}

BoundingBox3D vx::vis3d::Plane::getBoundingBox() {
  auto volumeObj = properties->sliceVolume();
  if (!volumeObj) {
    // If no volume is set, don't affect the bounding box
    return BoundingBox3D::empty();
  }
  auto volume = dynamic_cast<VolumeNode*>(volumeObj);
  if (!volume) {
    qWarning() << "Could not cast Node to VolumeNode";
    return BoundingBox3D::empty();
  }
  return volume->boundingBox();
}

void vx::vis3d::Plane::getClippingPlanes(QList<ClippingPlane>& planes) {
  auto plane = dynamic_cast<vx::PlaneNode*>(properties->plane());
  if (!plane) return;

  auto dir = properties->clippingDirection();
  if (dir == "de.uni_stuttgart.Voxie.Object3D.Plane.ClippingDirection.None") {
    // Do nothing
  } else if (dir ==
             "de.uni_stuttgart.Voxie.Object3D.Plane.ClippingDirection."
             "Positive") {
    planes << ClippingPlane(plane->plane()->getEquation());
  } else if (dir ==
             "de.uni_stuttgart.Voxie.Object3D.Plane.ClippingDirection."
             "Negative") {
    planes << ClippingPlane(-plane->plane()->getEquation());
  } else {
    qCritical() << "Invalid value for clipping direction";
  }
}

NODE_PROTOTYPE_IMPL_SEP(object3d_prop::Plane, vis3d::Plane)
