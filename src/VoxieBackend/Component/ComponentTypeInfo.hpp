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

#include <QtCore/QJsonObject>

namespace vx {
class Component;

// TODO: Should the ComponentType instance be available somewhere globally?

// Specialized for every component type, contains:
// static const char* name() => name of the component type
// static QList<std::tuple<QString, bool>> compatibilityNames() (optional)
template <typename T>
struct ComponentTypeInfo;

// Specialized for every component type, contains:
// static const char* jsonName() => name of the entry in extension files
// static QSharedPointer<vx::Component> parse(const QJsonObject& json) => parse
// static QList<std::tuple<QString, bool>> compatibilityJsonNames() (optional)
// extension JSON entry
template <typename T>
struct ComponentTypeInfoExt;
}  // namespace vx
