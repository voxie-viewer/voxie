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

#include "PlaneShader.hpp"

PlaneShader::PlaneShader() {}

QString PlaneShader::initialize() {
  initializeOpenGLFunctions();

  QString result = "";
  result += loadSimpleShaders();
  result += loadTextureShaders();
  result += setupUVBuffer();
  return result;
}

QString PlaneShader::loadTextureShaders() {
  const char* vshaderTexture =
      "#version 130\n"
      "\n"
      "attribute vec2 uvCoords;\n"
      "attribute vec3 vertexPosition_modelspace;\n"
      "\n"
      "uniform mat4 MVP;\n"
      "uniform bool drawTexture;\n"
      "\n"
      "out vec2 uv;\n"
      "\n"
      "void main() {\n"
      "    gl_Position = MVP * vec4(vertexPosition_modelspace,1);\n"
      "    uv = uvCoords;\n"
      "}\n";

  const char* fshaderTexture =
      "#version 130\n"
      "\n"
      "in vec2 uv;\n"
      "\n"
      "uniform sampler2D sliceTexture;\n"
      "uniform vec4 sliceColor;\n"
      "\n"
      "out vec4 color;\n"
      "void main(){\n"
      "    color = texture(sliceTexture, uv);\n"
      "    if(color.w == 0)\n"
      "        color = sliceColor;\n"
      "}\n";

  if (!textureProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                              vshaderTexture)) {
    return "Compiling texture vertex shader failed";
  }

  if (!textureProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                              fshaderTexture)) {
    return "Compiling texture fragment shader failed";
  }

  if (!textureProgram.link()) {
    return "Linking texture shaders failed";
  }

  MVP_ID = glGetUniformLocation(textureProgram.programId(), "MVP");
  sliceColorID = glGetUniformLocation(textureProgram.programId(), "sliceColor");
  sliceTextureID =
      glGetUniformLocation(textureProgram.programId(), "sliceTexture");

  if (MVP_ID == -1) qWarning() << "Failed to retrieve ID for MVP";
  if (sliceColorID == -1) qWarning() << "Failed to retrieve ID for sliceColor";
  if (sliceTextureID == -1)
    qWarning() << "Failed to retrieve ID for sliceTexture";

  vertexUVID = glGetAttribLocation(textureProgram.programId(), "uvCoords");
  vertexPosition_modelspaceID = glGetAttribLocation(
      textureProgram.programId(), "vertexPosition_modelspace");

  if (vertexPosition_modelspaceID == -1)
    qWarning() << "Failed to retrieve ID for vertexPosition_modelspace";
  if (vertexUVID == -1) qWarning() << "Failed to retrieve ID for uvCoords";

  if (!textureProgram.bind()) {
    return "Binding texture shaders failed";
  }
  return "";
}

QString PlaneShader::loadSimpleShaders() {
  const char* vshaderSimple =
      "#version 110\n"
      "\n"
      "attribute vec3 vertexPosition_modelspace;\n"
      "\n"
      "uniform mat4 MVP;\n"
      "\n"
      "void main() {\n"
      "    gl_Position = MVP * vec4(vertexPosition_modelspace,1);\n"
      "}\n";

  const char* fshaderSimple =
      "#version 110\n"
      "\n"
      "uniform vec4 sliceColor;\n"
      "\n"
      "void main(){\n"
      "   gl_FragColor = sliceColor;\n"
      "}\n";

  if (!planeProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                            vshaderSimple)) {
    return "Compiling slice vertex shader failed";
  }

  if (!planeProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                            fshaderSimple)) {
    return "Compiling slice fragment shader failed";
  }

  if (!planeProgram.link()) {
    return "Linking slice shaders failed";
  }

  sliceShader_MVP_ID = glGetUniformLocation(planeProgram.programId(), "MVP");
  sliceShader_sliceColorID =
      glGetUniformLocation(planeProgram.programId(), "sliceColor");

  if (sliceShader_MVP_ID == -1) qWarning() << "Failed to retrieve ID for MVP";
  if (sliceShader_sliceColorID == -1)
    qWarning() << "Failed to retrieve ID for sliceColor";

  sliceShader_vertexPosition_modelspaceID = glGetAttribLocation(
      planeProgram.programId(), "vertexPosition_modelspace");

  if (sliceShader_vertexPosition_modelspaceID == -1)
    qWarning() << "Failed to retrieve ID for vertexPosition_modelspace";

  if (!planeProgram.bind()) {
    return "Binding slice shaders failed";
  }
  return "";
}

void PlaneShader::draw(QSharedPointer<PlaneData> planeData,
                       const QMatrix4x4& modelViewProjectionMatrix,
                       const QVector4D& color) {
  // Create new vao to make sure it is valid for the current context
  QOpenGLVertexArrayObject vao;
  if (!vao.create()) {
    // return "Creating VAO failed";
    qWarning() << "Creating VAO failed";
  }

  vao.bind();

  if (planeData->drawTexture() && planeData->getTexture())
    drawTexture(planeData, modelViewProjectionMatrix, color);
  else
    drawSimple(planeData, modelViewProjectionMatrix, color);

  vao.release();
}

void PlaneShader::drawSimple(QSharedPointer<PlaneData> planeData,
                             const QMatrix4x4& modelViewProjectionMatrix,
                             const QVector4D& color) {
  auto vertexBuffer = planeData->getVertexBuffer();

  glUseProgram(planeProgram.programId());

  glUniformMatrix4fv(sliceShader_MVP_ID, 1, GL_FALSE,
                     modelViewProjectionMatrix.constData());
  glUniform4f(sliceShader_sliceColorID, color.x(), color.y(), color.z(),
              color.w());

  // 1st attribute buffer : vertices
  glEnableVertexAttribArray(sliceShader_vertexPosition_modelspaceID);

  if (!vertexBuffer->bind()) {
    qCritical() << "Binding slice vertex buffer failed 1";
    return;
  }
  glVertexAttribPointer(sliceShader_vertexPosition_modelspaceID,  // attribute
                        3,                                        // size
                        GL_FLOAT,                                 // type
                        GL_FALSE,                                 // normalized?
                        0,                                        // stride
                        (void*)0  // array buffer offset
  );
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(sliceShader_vertexPosition_modelspaceID);
}

void PlaneShader::drawTexture(QSharedPointer<PlaneData> planeData,
                              const QMatrix4x4& modelViewProjectionMatrix,
                              const QVector4D& color) {
  // Create new vao to make sure it is valid for the current context
  QOpenGLVertexArrayObject vao;
  if (!vao.create()) {
    // return "Creating VAO failed";
    qWarning() << "Creating VAO failed";
  }

  vao.bind();

  auto vertexBuffer = planeData->getVertexBuffer();

  glUseProgram(textureProgram.programId());

  glUniformMatrix4fv(MVP_ID, 1, GL_FALSE,
                     modelViewProjectionMatrix.constData());
  glUniform4f(sliceColorID, color.x(), color.y(), color.z(), color.w());

  // 1st attribute buffer : vertices
  glEnableVertexAttribArray(vertexPosition_modelspaceID);
  if (!vertexBuffer->bind()) {
    qCritical() << "Binding slice vertex buffer failed 2";
    return;
  }
  glVertexAttribPointer(vertexPosition_modelspaceID,  // attribute
                        3,                            // size
                        GL_FLOAT,                     // type
                        GL_FALSE,                     // normalized?
                        0,                            // stride
                        (void*)0                      // array buffer offset
  );

  // 2nd attribute buffer : uv coordinates
  glEnableVertexAttribArray(vertexUVID);
  if (!uvBuffer.bind()) {
    qCritical() << "Binding slice uv buffer failed";
    return;
  }
  glVertexAttribPointer(vertexUVID,  // attribute
                        2,           // size
                        GL_FLOAT,    // type
                        GL_FALSE,    // normalized?
                        0,           // stride
                        (void*)0     // array buffer offset
  );

  auto sliceTexture = planeData->getTexture();
  if (!sliceTexture) {
    qCritical() << "Texture data not found.";
    return;
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sliceTexture->textureID());
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sliceTexture->width(),
               sliceTexture->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
               sliceTexture->pixels());

  // XXX Why 0 and not textureID (which is actually 1)?
  glUniform1i(sliceTextureID, 0);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(vertexUVID);
  glDisableVertexAttribArray(vertexPosition_modelspaceID);

  vao.release();
}

void PlaneShader::updateBuffers(QSharedPointer<PlaneData> planeData) {
  updateBuffers(planeData, planeData->getBoundingRectangle());
}
void PlaneShader::updateBuffers(QSharedPointer<PlaneData> planeData,
                                const QRectF& bbox) {
  auto vertexBuffer = planeData->getVertexBuffer();

  if (!vertexBuffer->bind()) {
    qCritical() << "Binding slice vertex buffer failed 3";
    return;
  }
  auto bottom = bbox.top(), left = bbox.left(), right = bbox.right(),
       top = bbox.bottom();
  // Construct a quad of two triangles
  float vertexData[] = {
      (float)left,  (float)bottom, 0,  // Triangle 1, left bottom of quad
      (float)left,  (float)top,    0,  // Triangle 1, left top of quad
      (float)right, (float)top,    0,  // Triangle 1, right top of quad
      (float)left,  (float)bottom, 0,  // Triangle 2, left bottom of quad
      (float)right, (float)top,    0,  // Triangle 2, right top of quad
      (float)right, (float)bottom, 0   // Triangle 2, right bottom of quad
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof vertexData, vertexData, GL_STATIC_DRAW);
  vertexBuffer->release();
}

QString PlaneShader::setupUVBuffer() {
  if (!uvBuffer.create()) {
    return "Slice uv buffer create failed";
  }
  if (!uvBuffer.bind()) {
    return "Binding slice uv buffer failed";
  }

  // Construct the UV coordinates of the quad of two triangles.
  float uvData[] = {
      0, 0,  // Triangle 1, left bottom of quad
      0, 1,  // Triangle 1, left top of quad
      1, 1,  // Triangle 1, right top of quad
      0, 0,  // Triangle 2, left bottom of quad
      1, 1,  // Triangle 2, right top of quad
      1, 0   // Triangle 2, right bottom of quad
  };
  glBufferData(GL_ARRAY_BUFFER, sizeof uvData, uvData, GL_STATIC_DRAW);
  uvBuffer.release();
  return "";
}
