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

#include "Vis3DShaders.hpp"

#include <VoxieClient/Exception.hpp>

#include <VoxieBackend/Data/SurfaceData.hpp>

#include <qregularexpression.h>
#include <QFile>

#include <QtCore/QSharedPointer>

// The maximum hard coded limit of clip distances. This is needed because
// uniform arrays  in the shader have to be fixed size. (Actually it can be made
// dynamic if the vertex shader is re-compiled.
// XXX For some kind of reason the shader doesn't compile with higher values
// than 8
#define MAX_CLIP_DISTANCES 8

// Converts x to a string
#define TO_STR_HELPER(x) #x
#define TO_STR(x) TO_STR_HELPER(x)

using namespace vx;

Vis3DShaders::Vis3DShaders(QOpenGLFunctions* functions,
                           const QSharedPointer<SurfaceData>& surfaceAttributes,
                           const QString& shadingTechnique) {
  maxClipDistances_ = MAX_CLIP_DISTANCES;

  if (maxClipDistances_ > GL_MAX_CLIP_DISTANCES) {
    maxClipDistances_ = GL_MAX_CLIP_DISTANCES;
    qWarning()
        << "The maximum number of clip distances has been truncated to " TO_STR(
               GL_MAX_CLIP_DISTANCES);
  }

  auto renderTrianglesVertexSource =
      LoadShaderFile(":/PluginVis3D/Shader/RenderTriangles.vs.glsl",
                     surfaceAttributes, shadingTechnique);
  if (!renderTriangles_.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                                renderTrianglesVertexSource)) {
    throw Exception("de.uni_stuttgart.Voxie.Vis3D.ShaderCompilation",
                    "Compiling vertex shader failed");
  }

  auto renderTrianglesFragmentSource =
      LoadShaderFile(":/PluginVis3D/Shader/RenderTriangles.fs.glsl",
                     surfaceAttributes, shadingTechnique);
  if (!renderTriangles_.addShaderFromSourceCode(
          QOpenGLShader::Fragment, renderTrianglesFragmentSource)) {
    throw Exception("de.uni_stuttgart.Voxie.Vis3D.ShaderCompilation",
                    "Compiling fragment shader failed");
  }

  if (!renderTriangles_.link()) {
    throw Exception("de.uni_stuttgart.Voxie.Vis3D.ShaderCompilation",
                    "Linking shaders failed");
  }

  // TODO: move shaders into Qt Resources
  const char* pickVShader =
      //"#version 110\n"
      "#version 130\n"
      "\n"
      "attribute vec3 vertexPosition_modelspace;\n"
      "\n"
      "uniform mat4 MVP;\n"
      "flat out uint vertexID;\n"
      "\n"
      "void main() {\n"
      "    gl_Position = MVP * vec4(vertexPosition_modelspace,1);\n"
      "    vertexID = uint (gl_VertexID);\n"
      "}\n";

  if (!pickTriangles_.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                              pickVShader)) {
    throw Exception("de.uni_stuttgart.Voxie.Vis3D.ShaderCompilation",
                    "Compiling pick vertex shader failed");
  }

  const char* pickFShader =
      //"#version 330 core\n"
      "#version 130\n"
      "\n"
      "uniform int gObjectIndex;\n"
      "uniform int gDrawIndex;\n"
      "\n"
      "out uvec4 fragColor;\n"
      "\n"
      "flat in uint vertexID;\n"
      "\n"
      "void main(){\n"
      "\n"
      "    fragColor = uvec4(gObjectIndex, gDrawIndex, vertexID "
      "/*gl_PrimitiveID*/ + 1u, 0); // TODO: Use gl_PrimitiveID when it is "
      "available (and use the unmodified surface)"
      "\n"
      "}\n";
  if (!pickTriangles_.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                              pickFShader)) {
    throw Exception("de.uni_stuttgart.Voxie.Vis3D.ShaderCompilation",
                    "Compiling pick fragment shader failed");
  }

  if (!pickTriangles_.link()) {
    throw Exception("de.uni_stuttgart.Voxie.Vis3D.ShaderCompilation",
                    "Linking pick shaders failed");
  }

#define VAR(fname, fun, varname) \
  fname##_##varname##_ = functions->fun(fname##_.programId(), #varname)

  VAR(renderTriangles, glGetUniformLocation, MVP);
  VAR(renderTriangles, glGetUniformLocation, M);
  VAR(renderTriangles, glGetUniformLocation, lightPosition);
  VAR(renderTriangles, glGetUniformLocation, clippingPlanes);
  VAR(renderTriangles, glGetUniformLocation, clippingDirections);
  VAR(renderTriangles, glGetUniformLocation, cuttingLimit);
  VAR(renderTriangles, glGetUniformLocation, cuttingMode);
  VAR(renderTriangles, glGetUniformLocation, numClipDistances);
  VAR(renderTriangles, glGetUniformLocation, invertColor);
  VAR(renderTriangles, glGetUniformLocation, highlightedTriangle);
  VAR(renderTriangles, glGetUniformLocation, defaultFrontColor);
  VAR(renderTriangles, glGetUniformLocation, defaultBackColor);
  VAR(renderTriangles, glGetAttribLocation, vertexPosition_modelspace);
  VAR(renderTriangles, glGetAttribLocation, vertexNormal);

  VAR(pickTriangles, glGetAttribLocation, vertexPosition_modelspace);
  VAR(pickTriangles, glGetUniformLocation, MVP);
  VAR(pickTriangles, glGetUniformLocation, gObjectIndex);
  VAR(pickTriangles, glGetUniformLocation, gDrawIndex);

#undef VAR
}

Vis3DShaders::~Vis3DShaders() {}

QString Vis3DShaders::LoadShaderFile(
    QString path, const QSharedPointer<SurfaceData>& surfaceAttributes,
    const QString& shadingTechnique) {
  QFile f(path);
  if (!f.open(QFile::ReadOnly | QFile::Text)) return NULL;
  QTextStream in(&f);

  auto fullText = in.readAll();

  // TODO: Implement theses replacements in a better way

  fullText = fullText.replace("$maxClipDistances",
                              QString::number(maxClipDistances()));

  int useSmoothShading;
  if (shadingTechnique == "de.uni_stuttgart.Voxie.ShadingTechnique.Flat") {
    useSmoothShading = 0;
  } else if (shadingTechnique ==
             "de.uni_stuttgart.Voxie.ShadingTechnique.Smooth") {
    useSmoothShading = 1;
  } else {
    qWarning() << "Unknown shading technique" << shadingTechnique;
    useSmoothShading = 0;
  }
  fullText =
      fullText.replace("$useSmoothShading", QString::number(useSmoothShading));

  int foundIndex = 0;
  do {
    QRegularExpression regexHasTriangle1("\\$hasTriangleAttribute\\((.*)\\)");
    QRegularExpressionMatch match;
    foundIndex = fullText.indexOf(regexHasTriangle1, foundIndex + 1, &match);

    if (foundIndex >= 0) {
      auto attributeName = match.captured(1);
      int attributeExists = 0;

      if (surfaceAttributes) {
        auto attr = surfaceAttributes->getAttributeOrNull(attributeName);
        if (attr) {
          if (attr->kind() ==
              "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Triangle")
            attributeExists = 1;
        }
      }

      // replace $hasTriangleAttribute(...) with 0 or 1 depending on whether it
      // is in the surfaceAttributes or not
      QRegularExpression regexHasTriangle2("\\$hasTriangleAttribute\\(" +
                                           attributeName + "\\)");
      fullText =
          fullText.replace(regexHasTriangle2, QString::number(attributeExists));
    }
  } while (foundIndex >= 0);

  do {
    QRegularExpression regexHasVertex1("\\$hasVertexAttribute\\((.*)\\)");
    QRegularExpressionMatch match;
    foundIndex = fullText.indexOf(regexHasVertex1, foundIndex + 1, &match);

    if (foundIndex >= 0) {
      auto attributeName = match.captured(1);
      int attributeExists = 0;

      if (surfaceAttributes) {
        auto attr = surfaceAttributes->getAttributeOrNull(attributeName);
        if (attr) {
          // TODO: Vertex attributes
          if (attr->kind() ==
              "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Vertex")
            attributeExists = 1;
        }
      }

      // replace $hasVertexAttribute(...) with 0 or 1 depending on whether it is
      // in the surfaceAttributes or not
      QRegularExpression regexHasVertex2("\\$hasVertexAttribute\\(" +
                                         attributeName + "\\)");
      fullText =
          fullText.replace(regexHasVertex2, QString::number(attributeExists));
    }
  } while (foundIndex >= 0);

  return fullText;
}
