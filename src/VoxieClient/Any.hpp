/*
 * Copyright (c) 2014-2025 The Voxie Authors
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

#include <VoxieClient/VoxieClient.hpp>

#include <VoxieClient/ToString.hpp>
#include <VoxieClient/ToStringImpl.hpp>

#include <memory>

// TODO: Avoid including this here?
#include <QtCore/QString>

// This class implements a subset of std::any

namespace vx {
VOXIECLIENT_EXPORT Q_NORETURN void throwBadAnyCast(
    const std::type_info& expected, const std::type_info& actual);
VOXIECLIENT_EXPORT Q_NORETURN void throwNoToStringMethod(
    const std::type_info& type);

template <typename T>
struct InPlaceT {
  explicit InPlaceT() = default;
};

class Any;

template <typename T>
inline const T& any_cast(const Any& val);

template <>
struct VOXIECLIENT_EXPORT ToStringImpl<Any> {
  static const QString toString(const Any& value);
};

class Any {
  struct DataBase {
    virtual ~DataBase() {}
    virtual DataBase* copy() const = 0;
    virtual const std::type_info& type() const = 0;
    virtual bool hasToString() const = 0;
    virtual QString toString() const = 0;
  };

  template <typename T>
  struct Data : public DataBase {
    T value;

    Data(const T& value) : value(value) {}
    Data(T&& value) : value(std::move(value)) {}
    ~Data() override {}
    const std::type_info& type() const override { return typeid(T); }
  };

  template <typename T,
            bool hasToStringValue = Intern::has_tostringimpl<T>::value>
  struct DataImpl;

  template <typename T>
  struct DataImpl<T, false> : Data<T> {
    using Data<T>::Data;
    DataBase* copy() const override {
      // Note: This is in DataImpl, not in Data, to ensure that even if
      // hasToStringValue is different in different translation units, calling
      // copy() (i.e. the copy constructor of vx::Any) will not affect the value
      // of hasToString().
      return new DataImpl<T, false>(this->value);
    }
    QString toString() const override { throwNoToStringMethod(typeid(T)); }
    bool hasToString() const override { return false; }
  };

  template <typename T>
  struct DataImpl<T, true> : Data<T> {
    using Data<T>::Data;
    DataBase* copy() const override {
      // Note: This is in DataImpl, not in Data, to ensure that even if
      // hasToStringValue is different in different translation units, calling
      // copy() (i.e. the copy constructor of vx::Any) will not affect the value
      // of hasToString().
      return new DataImpl<T, true>(this->value);
    }
    QString toString() const override { return vx::toString(this->value); }
    bool hasToString() const override { return true; }
  };

  std::unique_ptr<DataBase> data;

  template <typename T>
  friend inline const T& any_cast(const Any& val);

  friend const QString ToStringImpl<Any>::toString(const Any& value);

 public:
  Any() : data() {}
  Any(const Any& val) : data(val.data ? val.data->copy() : nullptr) {}
  Any(Any&& val) noexcept : data(std::move(val.data)) {}
  // template <typename T>
  // Any(T&& value) : data(new
  // DataImpl<std::decay_t<T>>(std::forward<T>(value))) {}
  template <typename T, typename... Args>
  explicit Any(InPlaceT<T>, Args&&... args)
      : data(new DataImpl<std::decay_t<T>>(std::forward<Args>(args)...)) {}

  Any& operator=(const Any& val) {
    Any(val).swap(*this);
    return *this;
  }
  Any& operator=(Any&& val) {
    Any(std::move(val)).swap(*this);
    return *this;
  }

  bool has_value() const noexcept { return data.get(); }

  void swap(Any& other) noexcept { std::swap(data, other.data); }

  const std::type_info& type() const noexcept {
    return data.get() ? data->type() : typeid(void);
  }
};

inline void swap(Any& any1, Any& any2) noexcept { any1.swap(any2); }

template <typename T>
inline const T& any_cast(const Any& val) {
  const Any::Data<T>* ptr = dynamic_cast<const Any::Data<T>*>(val.data.get());
  if (!ptr)
    throwBadAnyCast(typeid(T),
                    val.data.get() ? val.data->type() : typeid(void));
  return ptr->value;
}

template <typename T, class... Args>
Any make_any(Args&&... args) {
  return Any(InPlaceT<T>(), std::forward<Args>(args)...);
}

namespace Intern {
VOXIECLIENT_EXPORT QString typeInfoToString(const std::type_info& info);
}
// TODO: Move somewhere else?
// TODO: Do something more sophisticated here?
template <typename T>
QString getTypeNameString() {
  return Intern::typeInfoToString(typeid(T));
}
}  // namespace vx
