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

#include <VoxieClient/Exception.hpp>
#include <VoxieClient/ToString.hpp>
#include <VoxieClient/ToStringImpl.hpp>
#include <VoxieClient/VoxieClient.hpp>

#include <QtDBus/QDBusUnixFileDescriptor>

#include <memory>

namespace vx {
class DBusValue;

class DBusSignatureBase {
 public:
  virtual ~DBusSignatureBase();

  virtual QString asString() = 0;
  // virtual DBusTypeKind kind() = 0;

  virtual DBusValue createEmptyValue() = 0;

  QDBusSignature asQDBusSignature();

  // Parse the first single complete type, throw if there is no valid single
  // complete type
  static std::shared_ptr<DBusSignatureBase> parseFirst(const QString& s,
                                                       int& index);
  // Parse the string as a single complete type, throw if the string is not a
  // single complete type
  static std::shared_ptr<DBusSignatureBase> parseSingle(const QString& s);
  // Parse the string as zero or more complete types, throw if the string cannot
  // be parsed that way
  static std::vector<std::shared_ptr<DBusSignatureBase>> parseList(
      const QString& s);
};

class DBusSignatureBasic : public DBusSignatureBase {
  char signatureCharacter_;

 public:
  explicit DBusSignatureBasic(char signatureCharacter);
  ~DBusSignatureBasic() override;

  char signatureCharacter() const { return signatureCharacter_; }

  QString asString() override;

  DBusValue createEmptyValue() override;
};

class DBusSignatureVariant : public DBusSignatureBase {
 public:
  explicit DBusSignatureVariant();
  ~DBusSignatureVariant() override;

  QString asString() override;

  DBusValue createEmptyValue() override;
};

class DBusSignatureArray : public DBusSignatureBase {
  std::shared_ptr<DBusSignatureBase> elementType_;

 public:
  explicit DBusSignatureArray(
      const std::shared_ptr<DBusSignatureBase>& elementType);
  ~DBusSignatureArray() override;

  const std::shared_ptr<DBusSignatureBase>& elementType() const {
    return elementType_;
  }

  QString asString() override;

  DBusValue createEmptyValue() override;
};

class DBusSignatureDictionary : public DBusSignatureBase {
  std::shared_ptr<DBusSignatureBase> keyType_;
  std::shared_ptr<DBusSignatureBase> valueType_;

 public:
  explicit DBusSignatureDictionary(
      const std::shared_ptr<DBusSignatureBase>& keyType,
      const std::shared_ptr<DBusSignatureBase>& valueType);
  ~DBusSignatureDictionary() override;

  const std::shared_ptr<DBusSignatureBase>& keyType() const { return keyType_; }
  const std::shared_ptr<DBusSignatureBase>& valueType() const {
    return valueType_;
  }

  QString asString() override;

  DBusValue createEmptyValue() override;
};

class DBusSignatureStruct : public DBusSignatureBase {
  std::vector<std::shared_ptr<DBusSignatureBase>> memberTypes_;

 public:
  explicit DBusSignatureStruct(
      const std::vector<std::shared_ptr<DBusSignatureBase>>& memberTypes);
  ~DBusSignatureStruct() override;

  const std::vector<std::shared_ptr<DBusSignatureBase>>& memberTypes() const {
    return memberTypes_;
  }

  QString asString() override;

  DBusValue createEmptyValue() override;
};

}  // namespace vx
