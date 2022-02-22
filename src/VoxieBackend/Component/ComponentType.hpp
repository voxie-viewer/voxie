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

#include <VoxieBackend/VoxieBackend.hpp>

#include <VoxieBackend/Component/ComponentTypeInfo.hpp>

#include <QtCore/QSharedPointer>
#include <QtCore/QString>

#include <array>
#include <functional>

namespace vx {
class Component;
template <typename T>
class SharedFunPtr;
class PropertyBase;

class VOXIEBACKEND_EXPORT ComponentType {
 public:
  using ParseFunctionPrototype = QSharedPointer<vx::Component>(
      const QJsonObject& json,
      const vx::SharedFunPtr<QList<QSharedPointer<PropertyBase>>(
          const QJsonObject&)>& parseProperties);

 private:
  QString name_;
  QString jsonName_;
  std::function<ParseFunctionPrototype> parseFunction_;
  QList<std::tuple<QString, bool>> compatibilityNames_;
  QList<std::tuple<QString, bool>> compatibilityJsonNames_;

  template <typename T>
  class has_compatibilityNames {
    using one = std::array<char, 1>;
    using two = std::array<char, 2>;

    template <typename C>
    static one test(decltype(&C::compatibilityNames));
    template <typename C>
    static two test(...);

   public:
    static constexpr bool value = sizeof(test<T>(0)) == sizeof(one);
  };

  template <typename T>
  class has_compatibilityJsonNames {
    using one = std::array<char, 1>;
    using two = std::array<char, 2>;

    template <typename C>
    static one test(decltype(&C::compatibilityJsonNames));
    template <typename C>
    static two test(...);

   public:
    static constexpr bool value = sizeof(test<T>(0)) == sizeof(one);
  };

  template <typename T, bool has = has_compatibilityNames<T>::value>
  struct get_compatibilityNames;
  template <typename T>
  struct get_compatibilityNames<T, false> {
    static QList<std::tuple<QString, bool>> get() { return {}; }
  };
  template <typename T>
  struct get_compatibilityNames<T, true> {
    static QList<std::tuple<QString, bool>> get() {
      return T::compatibilityNames();
    }
  };

  template <typename T, bool has = has_compatibilityJsonNames<T>::value>
  struct get_compatibilityJsonNames;
  template <typename T>
  struct get_compatibilityJsonNames<T, false> {
    static QList<std::tuple<QString, bool>> get() { return {}; }
  };
  template <typename T>
  struct get_compatibilityJsonNames<T, true> {
    static QList<std::tuple<QString, bool>> get() {
      return T::compatibilityJsonNames();
    }
  };

 public:
  ComponentType(const QString& name, const QString& jsonName,
                const std::function<ParseFunctionPrototype>& parseFunction,
                const QList<std::tuple<QString, bool>>& compatibilityNames,
                const QList<std::tuple<QString, bool>>& compatibilityJsonNames);
  ~ComponentType();

  template <typename T>
  static ComponentType create() {
    return ComponentType(
        ComponentTypeInfo<T>::name(), ComponentTypeInfoExt<T>::jsonName(),
        &ComponentTypeInfoExt<T>::parse,
        get_compatibilityNames<ComponentTypeInfo<T>>::get(),
        get_compatibilityJsonNames<ComponentTypeInfoExt<T>>::get());
  }

  template <typename T>
  static ComponentType createNoExt() {
    return ComponentType(ComponentTypeInfo<T>::name(), "", nullptr,
                         get_compatibilityNames<ComponentTypeInfo<T>>::get(),
                         {});
  }

  const QString& name() const { return name_; }
  const QString& jsonName() const { return jsonName_; }
  const std::function<ParseFunctionPrototype>& parseFunction() const {
    return parseFunction_;
  }
  const QList<std::tuple<QString, bool>>& compatibilityNames() const {
    return compatibilityNames_;
  }
  const QList<std::tuple<QString, bool>>& compatibilityJsonNames() const {
    return compatibilityJsonNames_;
  }
};
}  // namespace vx
