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

#include "OpenGLWidget.hpp"

#include <VoxieClient/Exception.hpp>
#include <VoxieClient/QtUtil.hpp>

#include <Voxie/DebugOptions.hpp>
#include <Voxie/IVoxie.hpp>

#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>

#include <QtCore/QDebug>

// TODO: Check that this is working with QOpenGLWindow / make it work again

using namespace vx::visualization;

static QString getGLErrorString(GLenum error) {
  switch (error) {
    case GL_NO_ERROR:
      return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
      return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
      return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
      return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
      return "GL_OUT_OF_MEMORY";
    default:
      return QString::number(error);
  }
}
bool OpenGLWidget::checkOpenGLStatus() {
  GLenum error = glGetError();
  if (error == GL_NO_ERROR) return true;

  QString errorStr;
  int count = 0;
  while (error != GL_NO_ERROR) {
    if (count != 0) errorStr += ",";
    errorStr += getGLErrorString(error);

    if (count > 20)
      break;  // Prevent endless loop if glGetError() doesn't clear the error

    error = glGetError();
    count++;
  }
  qWarning() << "Got OpenGL error:" << errorStr;
  return false;
}

OpenGLWidget::OpenGLWidget(QWidget* parent)
    : QOpenGLWidget(parent),
      fWidthPhys(1),
      fHeightPhys(1),
      fWidthDIP(1),
      fHeightDIP(1) {}

void OpenGLWidget::initializeGL() {
#define FAIL(x)       \
  do {                \
    initError = x;    \
    qCritical() << x; \
    return;           \
  } while (0)

  if (voxieRoot().disableOpenGL()) {
    FAIL("OpenGL support is disabled from the command line");
  }

  if (!context()->isValid()) {
    FAIL("OpenGL context is not valid");
  }

  /*
  auto logger = new QOpenGLDebugLogger(this);
  logger->initialize();
  connect(logger, &QOpenGLDebugLogger::messageLogged, [] (const
  QOpenGLDebugMessage& msg) { qWarning() << "OpenGL Debug message:" << msg; });
  logger->startLogging();
  */

  initializeOpenGLFunctions();  // QOpenGLFunctions::initializeOpenGLFunctions()
                                // returns a void
  /*
  if (!initializeOpenGLFunctions()) {
      FAIL("Could not initialize OpenGL functions");
  }
  */

  QString error = initialize();
  if (error != "") FAIL(error);

  if (!checkOpenGLStatus()) FAIL("OpenGL error during initialization");

  initialized_ = true;

#undef FAIL
}

void OpenGLWidget::resizeGL(int w, int h) {
  this->fWidthDIP = static_cast<float>(w);
  this->fHeightDIP = static_cast<float>(h);

  // https://doc.qt.io/qt-5/qwindow.html#devicePixelRatio
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
  double factor = this->window()->devicePixelRatioF();
#else
  double factor = this->window()->devicePixelRatio();
#endif
  this->fWidthPhys = this->fWidthDIP * factor;
  this->fHeightPhys = this->fHeightDIP * factor;

  this->fWidthPhys = std::max<float>(this->fWidthPhys, 1);
  this->fHeightPhys = std::max<float>(this->fHeightPhys, 1);
  this->fWidthDIP = std::max<float>(this->fWidthDIP, 1);
  this->fHeightDIP = std::max<float>(this->fHeightDIP, 1);

  // TODO: Is there a better way to get the phsical pixel count without
  // rounding?
  this->iWidthPhys = std::round(fWidthPhys);
  this->iHeightPhys = std::round(fHeightPhys);

  if (!initialized()) return;

  glViewport(0, 0, this->fWidthPhys, this->fHeightPhys);
}

void OpenGLWidget::paintGL() {
  if (!initialized()) return;

  paint();
}

void OpenGLWidget::paintEvent(QPaintEvent* event) {
  if (initialized()) {
    checkOpenGLStatus();
    QOpenGLWidget::paintEvent(event);
    checkOpenGLStatus();
    return;
  }

  // return;
  QPainter painter;
  painter.begin(this);
  painter.drawText(QRect(QPoint(0, 0), this->size()), Qt::AlignCenter,
                   "Error initializing OpenGL:\n" + initError);
  painter.end();
}

// This code will coalesce resize events. This seems to be needed on Wayland for
// some reason (at least for Qt 5.15.8)
void OpenGLWidget::resizeEvent(QResizeEvent* event) {
  if (vx::debug_option::Log_Workaround_CoalesceOpenGLResize()->enabled())
    qDebug() << "OpenGLWidget::resizeEvent start";
  if (vx::debug_option::Workaround_CoalesceOpenGLResize()->enabled()) {
    resizePending = true;
    vx::enqueueOnThread(this, [this, e = QResizeEvent(event->size(),
                                                      event->oldSize())]() {
      if (vx::debug_option::Log_Workaround_CoalesceOpenGLResize()->enabled())
        qDebug() << "OpenGLWidget::resizeEvent cb" << resizePending;
      if (!resizePending) return;
      resizePending = false;
      QResizeEvent e2(e);
      QOpenGLWidget::resizeEvent(&e2);
      if (vx::debug_option::Log_Workaround_CoalesceOpenGLResize()->enabled())
        qDebug() << "OpenGLWidget::resizeEvent cb done";
    });
  } else {
    QOpenGLWidget::resizeEvent(event);
  }
  if (vx::debug_option::Log_Workaround_CoalesceOpenGLResize()->enabled())
    qDebug() << "OpenGLWidget::resizeEvent end";
}

OpenGLDrawUtils::OpenGLDrawUtils(QObject* parent) : QObject(parent) {}
OpenGLDrawUtils::~OpenGLDrawUtils() {}

void OpenGLDrawUtils::initialize() {
  initializeOpenGLFunctions();  // QOpenGLFunctions::initializeOpenGLFunctions()
                                // returns a void

  /*
  if (!vao.create()) {
      //throw vx::Exception("de.uni_stuttgart.Voxie.Error", "Creating VAO
  failed"); qWarning() << "Creating VAO failed";
  }
  */

  // Simple shader

  if (!programSimple.addShaderFromSourceFile(
          QOpenGLShader::Vertex, ":/Voxie/Vis/Shader/SimpleShader.vs.glsl")) {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Compiling vertex shader failed");
  }

  if (!programSimple.addShaderFromSourceFile(
          QOpenGLShader::Fragment, ":/Voxie/Vis/Shader/SimpleShader.fs.glsl")) {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Compiling fragment shader failed");
  }

  if (!programSimple.link()) {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Linking shaders failed");
  }

  simple_MVP_ID = glGetUniformLocation(programSimple.programId(), "MVP");

  simple_vertexPosition_modelspaceID = glGetAttribLocation(
      programSimple.programId(), "vertexPosition_modelspace");
  simple_vertexColorID =
      glGetAttribLocation(programSimple.programId(), "vertexColor");

  if (!programSimple.bind()) {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Binding shaders failed");
  }

  // Flat shader

  if (!programFlat.addShaderFromSourceFile(
          QOpenGLShader::Vertex, ":/Voxie/Vis/Shader/FlatShader.vs.glsl")) {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Compiling vertex shader failed");
  }

  if (!programFlat.addShaderFromSourceFile(
          QOpenGLShader::Fragment, ":/Voxie/Vis/Shader/FlatShader.fs.glsl")) {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Compiling fragment shader failed");
  }

  if (!programFlat.link()) {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Linking shaders failed");
  }

  flat_MVP_ID = glGetUniformLocation(programFlat.programId(), "MVP");
  flat_M_ID = glGetUniformLocation(programFlat.programId(), "M");
  flat_lightPositionID =
      glGetUniformLocation(programFlat.programId(), "lightPosition");

  flat_vertexPosition_modelspaceID =
      glGetAttribLocation(programFlat.programId(), "vertexPosition_modelspace");
  flat_vertexNormalID =
      glGetAttribLocation(programFlat.programId(), "vertexNormal");
  flat_vertexColorID =
      glGetAttribLocation(programFlat.programId(), "vertexColor");

  if (!programFlat.bind()) {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Binding shaders failed");
  }

  // End of shaders

  if (!vertexbuffer.create()) {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Vertex buffer create failed");
  }

  if (!normalbuffer.create()) {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Normal buffer create failed");
  }

  if (!colorbuffer.create()) {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Color buffer create failed");
  }
}

void OpenGLDrawUtils::PrimitiveBuffer::push(std::vector<GLfloat>& array,
                                            const QVector3D& data) {
  array.push_back(data.x());
  array.push_back(data.y());
  array.push_back(data.z());
}

void OpenGLDrawUtils::PrimitiveBuffer::push(std::vector<GLfloat>& array,
                                            const vx::Color& data) {
  array.push_back(data.red());
  array.push_back(data.green());
  array.push_back(data.blue());
  array.push_back(data.alpha());
}

void OpenGLDrawUtils::PrimitiveBuffer::addQuad(const vx::Color& color,
                                               const QVector3D& a,
                                               const QVector3D& b,
                                               const QVector3D& c,
                                               const QVector3D& d) {
  if (vertices.size() == 0) {
    mode = GL_TRIANGLES;
  } else if (mode != GL_TRIANGLES) {
    qCritical() << "OpenGLDrawUtils::PrimitiveBuffer::addQuad(): Buffer does "
                   "not contain triangles";
    return;
  }

  for (int i = 0; i < 6; i++) push(colors, color);

  push(vertices, a);
  push(vertices, b);
  push(vertices, c);

  push(vertices, a);
  push(vertices, c);
  push(vertices, d);
}

void OpenGLDrawUtils::PrimitiveBuffer::addTriangle(const vx::Color& color,
                                                   const QVector3D& a,
                                                   const QVector3D& b,
                                                   const QVector3D& c) {
  if (vertices.size() == 0) {
    mode = GL_TRIANGLES;
  } else if (mode != GL_TRIANGLES) {
    qCritical() << "OpenGLDrawUtils::PrimitiveBuffer::addQuad(): Buffer does "
                   "not contain triangles";
    return;
  }

  for (int i = 0; i < 3; i++) push(colors, color);

  push(vertices, a);
  push(vertices, b);
  push(vertices, c);
}

void OpenGLDrawUtils::PrimitiveBuffer::addLine(const vx::Color& color,
                                               const QVector3D& a,
                                               const QVector3D& b) {
  if (vertices.size() == 0) {
    mode = GL_LINES;
  } else if (mode != GL_LINES) {
    qCritical() << "OpenGLDrawUtils::PrimitiveBuffer::addQuad(): Buffer does "
                   "not contains triangles";
    return;
  }

  push(colors, color);
  push(colors, color);

  push(vertices, a);
  push(vertices, b);
}

void OpenGLDrawUtils::PrimitiveBuffer::clear() {
  vertices.clear();
  colors.clear();
}

// Like QVector3D::normalized(), but with a smaller minimum size
// TODO: Is this needed here? Duplicate from src/PluginVis3D/Surface.cpp
static QVector3D normalized(const QVector3D& value) {
  double absSquared = double(value.x()) * double(value.x()) +
                      double(value.y()) * double(value.y()) +
                      double(value.z()) * double(value.z());
  if (absSquared >= 1e-50) {  // minimum length 1e-25
    double len = std::sqrt(absSquared);
    return QVector3D(float(double(value.x()) / len),
                     float(double(value.y()) / len),
                     float(double(value.z()) / len));
  } else {
    return QVector3D();
  }
}

void OpenGLDrawUtils::draw(GLenum mode, const std::vector<GLfloat>& vertices,
                           const std::vector<GLfloat>& colors,
                           const QMatrix4x4& modelMatrix,
                           const QMatrix4x4& modelViewProjectionMatrix,
                           const QVector4D* lightPosition) {
  // Create new vao to make sure it is valid for the current context
  QOpenGLVertexArrayObject vao;
  if (!vao.create()) {
    // return "Creating VAO failed";
    qWarning() << "Creating VAO failed";
  }

  vao.bind();

  if (!vertexbuffer.bind()) {
    qCritical() << "Binding vertex buffer failed";
    return;
  }
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
               vertices.data(), GL_STATIC_DRAW);
  vertexbuffer.release();

  if (lightPosition) {
    if (mode != GL_TRIANGLES) {
      qCritical() << "Attempt to draw with light in non-GL_TRIANGLES mode";
      return;
    }
    if (vertices.size() % 9 != 0) {
      qCritical() << "Number of vertices entries is not a multiple of 9";
      return;
    }
    size_t count = vertices.size() / 9;

    std::vector<GLfloat> normals(count * 9);
    for (size_t i = 0; i < count; i++) {
      size_t base = i * 9;
      QVector3D v1(vertices[base + 0], vertices[base + 1], vertices[base + 2]);
      QVector3D v2(vertices[base + 3], vertices[base + 4], vertices[base + 5]);
      QVector3D v3(vertices[base + 6], vertices[base + 7], vertices[base + 8]);

      // Calculate triangle normal
      QVector3D normal = normalized(QVector3D::crossProduct(v2 - v3, v3 - v1));
      // qDebug() << "Normal" << v1 << v2 << v3 << "=>" << normal;

      // Fill in the normal for all three vertices
      // TODO: Avoid this? Probably only needed for the controlling vertex.
      for (size_t j = 0; j < 3; j++) {
        normals[i * 9 + j * 3 + 0] = normal.x();
        normals[i * 9 + j * 3 + 1] = normal.y();
        normals[i * 9 + j * 3 + 2] = normal.z();
      }
    }

    if (!normalbuffer.bind()) {
      qCritical() << "Binding normal buffer failed";
      return;
    }
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float),
                 normals.data(), GL_STATIC_DRAW);
    normalbuffer.release();
  }

  if (!colorbuffer.bind()) {
    qCritical() << "Binding color buffer failed";
    return;
  }
  glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(),
               GL_STATIC_DRAW);
  colorbuffer.release();

  if (lightPosition) {
    // qDebug() << "Light position" << *lightPosition;

    glUseProgram(programFlat.programId());

    glUniformMatrix4fv(flat_M_ID, 1, GL_FALSE, modelMatrix.constData());
    glUniformMatrix4fv(flat_MVP_ID, 1, GL_FALSE,
                       modelViewProjectionMatrix.constData());
    glUniform4f(flat_lightPositionID, lightPosition->x(), lightPosition->y(),
                lightPosition->z(), lightPosition->w());

    // 1st attribute buffer : vertices
    glEnableVertexAttribArray(flat_vertexPosition_modelspaceID);
    if (!vertexbuffer.bind()) {
      qCritical() << "Binding vertex buffer failed";
      return;
    }
    glVertexAttribPointer(flat_vertexPosition_modelspaceID,  // attribute
                          3,                                 // size
                          GL_FLOAT,                          // type
                          GL_FALSE,                          // normalized?
                          0,                                 // stride
                          (void*)0  // array buffer offset
    );

    // 2nd attribute buffer : normals
    glEnableVertexAttribArray(flat_vertexNormalID);
    if (!normalbuffer.bind()) {
      qCritical() << "Binding normal buffer failed";
      return;
    }
    glVertexAttribPointer(flat_vertexNormalID,  // attribute
                          3,                    // size
                          GL_FLOAT,             // type
                          GL_FALSE,             // normalized?
                          0,                    // stride
                          (void*)0              // array buffer offset
    );

    // 3rd attribute buffer : colors
    glEnableVertexAttribArray(flat_vertexColorID);
    if (!colorbuffer.bind()) {
      qCritical() << "Binding color buffer failed";
      return;
    }
    glVertexAttribPointer(flat_vertexColorID,  // attribute
                          4,                   // size
                          GL_FLOAT,            // type
                          GL_FALSE,            // normalized?
                          0,                   // stride
                          (void*)0             // array buffer offset
    );
  } else {
    glUseProgram(programSimple.programId());

    glUniformMatrix4fv(simple_MVP_ID, 1, GL_FALSE,
                       modelViewProjectionMatrix.constData());

    // 1st attribute buffer : vertices
    glEnableVertexAttribArray(simple_vertexPosition_modelspaceID);
    if (!vertexbuffer.bind()) {
      qCritical() << "Binding vertex buffer failed";
      return;
    }
    glVertexAttribPointer(simple_vertexPosition_modelspaceID,  // attribute
                          3,                                   // size
                          GL_FLOAT,                            // type
                          GL_FALSE,                            // normalized?
                          0,                                   // stride
                          (void*)0  // array buffer offset
    );

    // 2nd attribute buffer : colors
    glEnableVertexAttribArray(simple_vertexColorID);
    if (!colorbuffer.bind()) {
      qCritical() << "Binding color buffer failed";
      return;
    }
    glVertexAttribPointer(simple_vertexColorID,  // attribute
                          4,                     // size
                          GL_FLOAT,              // type
                          GL_FALSE,              // normalized?
                          0,                     // stride
                          (void*)0               // array buffer offset
    );
  }

  glDrawArrays(mode, 0, vertices.size() / 3);

  if (lightPosition) {
    glDisableVertexAttribArray(flat_vertexPosition_modelspaceID);
    glDisableVertexAttribArray(flat_vertexNormalID);
    glDisableVertexAttribArray(flat_vertexColorID);
  } else {
    glDisableVertexAttribArray(simple_vertexPosition_modelspaceID);
    glDisableVertexAttribArray(simple_vertexColorID);
  }

  vao.release();
}

void OpenGLDrawUtils::draw(const PrimitiveBuffer& buffer,
                           const QMatrix4x4& modelMatrix,
                           const QMatrix4x4& modelViewProjectionMatrix,
                           const QVector4D* lightPosition) {
  if (buffer.vertices.size() == 0) return;

  draw(buffer.mode, buffer.vertices, buffer.colors, modelMatrix,
       modelViewProjectionMatrix, lightPosition);
}

OpenGLDrawWidget::OpenGLDrawWidget(QWidget* parent)
    : OpenGLWidget(parent), utils(new OpenGLDrawUtils(this)) {}

QString OpenGLDrawWidget::initialize() {
  /*
  QString err = OpenGLWidget::initialize();
  if (err != "")
    return err;
  */

  try {
    utils->initialize();
  } catch (vx::Exception& e) {
    return e.message();
  }

  return "";
}

void OpenGLDrawWidget::draw(const PrimitiveBuffer& buffer,
                            const QMatrix4x4& modelMatrix,
                            const QMatrix4x4& modelViewProjectionMatrix,
                            const QVector4D* lightPosition) {
  utils->draw(buffer, modelMatrix, modelViewProjectionMatrix, lightPosition);
}
