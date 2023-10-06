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

#include <Voxie/Voxie.hpp>

#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLVertexArrayObject>

#include <QtWidgets/QOpenGLWidget>

namespace vx {
namespace visualization {

class VOXIECORESHARED_EXPORT OpenGLWidget : public QOpenGLWidget,
                                            protected QOpenGLFunctions {
  Q_OBJECT

 private:
  float fWidthPhys, fHeightPhys;
  float fWidthDIP, fHeightDIP;
  int iWidthPhys, iHeightPhys;

  bool initialized_ = false;
  QString initError;

 private:
  int width() const = delete;
  int height() const = delete;

 public:
  explicit OpenGLWidget(QWidget* parent = 0);

  float widthPhys() const { return fWidthPhys; }
  float heightPhys() const { return fHeightPhys; }
  float widthDIP() const { return fWidthDIP; }
  float heightDIP() const { return fHeightDIP; }
  int widthPhysInt() const { return iWidthPhys; }
  int heightPhysInt() const { return iHeightPhys; }

  bool initialized() const { return initialized_; }

 protected:
  virtual void initializeGL() override;
  virtual void resizeGL(int w, int h) override;
  virtual void paintGL() override;

  virtual void paintEvent(QPaintEvent* event) override;
  virtual void resizeEvent(QResizeEvent* event) override;

  virtual QString initialize() = 0;
  virtual void paint() = 0;

  bool checkOpenGLStatus();

 private:
  bool resizePending = false;
};

class VOXIECORESHARED_EXPORT OpenGLDrawUtils : public QObject,
                                               protected QOpenGLFunctions {
 private:
  // QOpenGLVertexArrayObject vao;
  QOpenGLShaderProgram program;
  GLuint MVP_ID;
  GLuint vertexPosition_modelspaceID;
  GLuint vertexColorID;
  QOpenGLBuffer vertexbuffer;
  QOpenGLBuffer colorbuffer;

 public:
  OpenGLDrawUtils(QObject* parent = 0);
  ~OpenGLDrawUtils();

  void initialize();

  class VOXIECORESHARED_EXPORT PrimitiveBuffer {
   public:
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> colors;
    GLenum mode;

    static void push(std::vector<GLfloat>& array, const QVector3D& data);
    static void push(std::vector<GLfloat>& array, const QVector4D& data);

    // Adds two triangles
    void addQuad(const QVector4D& color, const QVector3D& a, const QVector3D& b,
                 const QVector3D& c, const QVector3D& d);

    /**
     * @brief addQuad Adds one triangle to the buffer.
     * @param color
     * @param a
     * @param b
     * @param c
     */
    void addTriangle(const QVector4D& color, const QVector3D& a,
                     const QVector3D& b, const QVector3D& c);

    void addLine(const QVector4D& color, const QVector3D& a,
                 const QVector3D& b);

    void clear();
  };

  /**
   * Draw primitives. The vertices are taken from the vertices vector,
   * (3 values per vertex), the colors from the colors vector (4 values per
   * vertex, including alpha value).
   */
  void draw(GLenum mode, const std::vector<GLfloat>& vertices,
            const std::vector<GLfloat>& colors,
            const QMatrix4x4& modelViewProjectionMatrix);

  void draw(const PrimitiveBuffer& buffer,
            const QMatrix4x4& modelViewProjectionMatrix);
};

// Subclass of OpenGLWidget which provides some basic 3D drawing operations
class VOXIECORESHARED_EXPORT OpenGLDrawWidget : public OpenGLWidget {
  Q_OBJECT

  OpenGLDrawUtils* utils;

 public:
  explicit OpenGLDrawWidget(QWidget* parent = 0);

  virtual QString initialize() override;

  typedef OpenGLDrawUtils::PrimitiveBuffer PrimitiveBuffer;

  void draw(const PrimitiveBuffer& buffer,
            const QMatrix4x4& modelViewProjectionMatrix);
};

}  // namespace visualization
}  // namespace vx
