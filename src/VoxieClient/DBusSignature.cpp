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

#include "DBusSignature.hpp"

#include <VoxieClient/DBusValue.hpp>
#include <VoxieClient/DBusVariant.hpp>
#include <VoxieClient/Format.hpp>

using namespace vx;

DBusSignatureBase::~DBusSignatureBase() {}

QDBusSignature DBusSignatureBase::asQDBusSignature() {
  return QDBusSignature(this->asString());
}

DBusSignatureBasic::DBusSignatureBasic(char signatureCharacter)
    : signatureCharacter_(signatureCharacter) {
  switch (signatureCharacter_) {
#define MACRO(sig, T) case sig:
    VX_DBUS_FOR_EACH_BASIC_TYPE(MACRO)
#undef MACRO
    break;

    default:
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Invalid character for basic type");
  }
}
DBusSignatureBasic::~DBusSignatureBasic() {}

QString DBusSignatureBasic::asString() {
  char s[] = {this->signatureCharacter(), 0};
  return QString(s);
}

DBusValue DBusSignatureBasic::createEmptyValue() {
  switch (signatureCharacter_) {
#define MACRO(sig, T) \
  case sig:           \
    return DBusValue::createBasic<T>(T());
    VX_DBUS_FOR_EACH_BASIC_TYPE(MACRO)
#undef MACRO

    default:
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Invalid character for basic type");
  }
}

DBusSignatureVariant::DBusSignatureVariant() {}
DBusSignatureVariant::~DBusSignatureVariant() {}

QString DBusSignatureVariant::asString() { return "v"; }

DBusValue DBusSignatureVariant::createEmptyValue() {
  // Create a variant with 'false' in it
  return DBusValue::createVariant(
      DBusVariant(DBusValue::createBasic<bool>(false)));
}

DBusSignatureArray::DBusSignatureArray(
    const std::shared_ptr<DBusSignatureBase>& elementType)
    : elementType_(elementType) {
  if (!elementType_)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "DBusSignatureArray: elementType is nullptr");

  // TODO: Check nesting limit?
}
DBusSignatureArray::~DBusSignatureArray() {}

QString DBusSignatureArray::asString() {
  return "a" + this->elementType()->asString();
}

DBusValue DBusSignatureArray::createEmptyValue() {
  return DBusValue::createArray(this->elementType()->asQDBusSignature(), {});
}

DBusSignatureDictionary::DBusSignatureDictionary(
    const std::shared_ptr<DBusSignatureBase>& keyType,
    const std::shared_ptr<DBusSignatureBase>& valueType)
    : keyType_(keyType), valueType_(valueType) {
  if (!keyType_)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "DBusSignatureDictionary: keyType is nullptr");
  if (!valueType_)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "DBusSignatureDictionary: valueType is nullptr");

  // TODO: Check nesting limit for keys and values?
}
DBusSignatureDictionary::~DBusSignatureDictionary() {}

QString DBusSignatureDictionary::asString() {
  return "a{" + this->keyType()->asString() + this->valueType()->asString() +
         "}";
}

DBusValue DBusSignatureDictionary::createEmptyValue() {
  return DBusValue::createDictionary(this->keyType()->asQDBusSignature(),
                                     this->valueType()->asQDBusSignature(), {});
}

DBusSignatureStruct::DBusSignatureStruct(
    const std::vector<std::shared_ptr<DBusSignatureBase>>& memberTypes)
    : memberTypes_(memberTypes) {
  if (memberTypes.size() == 0)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "DBusSignatureStruct: Got empty struct");
  for (const auto& memberType : memberTypes_)
    if (!memberType)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "DBusSignatureDictionary: memberType is nullptr");

  // TODO: Check nesting limit for members?
}
DBusSignatureStruct::~DBusSignatureStruct() {}

QString DBusSignatureStruct::asString() {
  QString res = "{";
  for (const auto& memberType : this->memberTypes())
    res += memberType->asString();
  res += "}";
  return res;
}

DBusValue DBusSignatureStruct::createEmptyValue() {
  std::vector<DBusValue> members;
  for (const auto& memberType : this->memberTypes())
    members.push_back(memberType->createEmptyValue());
  return DBusValue::createStruct(members);
}

std::shared_ptr<DBusSignatureBase> DBusSignatureBase::parseFirst(
    const QString& s, int& index) {
  if (index < 0 || index > s.size())
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Invalid index");

  if (index >= s.size())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "DBusSignatureBase::parseFirst: Reached end of string "
                        "while parsing type");
  switch (s[index].toLatin1()) {
    // Basic types
#define MACRO(sig, T) case sig:
    VX_DBUS_FOR_EACH_BASIC_TYPE(MACRO)
#undef MACRO
    {
      auto type = std::make_shared<DBusSignatureBasic>(s[index].toLatin1());
      index += 1;
      return type;
    }
    break;

    case 'v': {
      index += 1;
      return std::make_shared<DBusSignatureVariant>();
    } break;

    case 'a': {
      index += 1;
      if (index < s.size() && s[index] == '{') {
        // Dictionary

        // Skip over '{'
        index += 1;

        auto keyType = parseFirst(s, index);
        auto valueType = parseFirst(s, index);
        if (index >= s.size())
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.Error",
              "DBusSignatureBase::parseFirst: Reached end of string "
              "while parsing type (end of dictionary)");
        if (s[index] != '}')
          throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                              "DBusSignatureBase::parseFirst: Got invalid "
                              "character while looking for end of dictionary");
        // Skip over '}'
        index += 1;
        return std::make_shared<DBusSignatureDictionary>(keyType, valueType);
      } else {
        // Array (or end of string)
        auto elementType = parseFirst(s, index);
        return std::make_shared<DBusSignatureArray>(elementType);
      }
    } break;

    case '(': {
      index += 1;
      std::vector<std::shared_ptr<DBusSignatureBase>> memberTypes;
      while (index >= s.size() || s[index] != ')')
        memberTypes.push_back(parseFirst(s, index));
      // s[index] is ')', skip over that
      index += 1;
      // Note: This will throw if the list of memberTypes is empty
      return std::make_shared<DBusSignatureStruct>(memberTypes);
    } break;

    default:
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          vx::format("Invalid character in DBus signature at position {}",
                     index));
  }
}

std::shared_ptr<DBusSignatureBase> DBusSignatureBase::parseSingle(
    const QString& s) {
  auto types = parseList(s);

  if (types.size() == 0)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "DBusSignatureBase::parseSingle(): Got an empty list "
                        "of single complete types");
  if (types.size() > 1)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "DBusSignatureBase::parseSingle(): Got multiple single complete types");
  return types[0];
}

std::vector<std::shared_ptr<DBusSignatureBase>> DBusSignatureBase::parseList(
    const QString& s) {
  std::vector<std::shared_ptr<DBusSignatureBase>> result;
  int index = 0;
  while (index < s.size()) result.push_back(parseFirst(s, index));
  return result;
}
