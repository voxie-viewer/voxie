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

#include <VoxieBackend/Property/PropertyType.hpp>

#include <Voxie/Data/TableData.hpp>

/**
 * Contains utility functions for handling table columns and values.
 */
namespace TableUtils {

// TODO: possibly move this function into property types themselves
/**
 * Returns true if the specified property type is numeric (i.e. always
 * convertible to double).
 *
 * Float, Int and Boolean are numeric types.
 */
bool isNumericType(const vx::PropertyType& type);

// TODO: possibly move this function into property types themselves
/**
 * Returns true if the specified property type is meaningfully comparable
 * (either convertible to numeric types, or has a meaningful lexical
 * comparison defined).
 *
 * All numeric types are comparable, as well as String and Enumeration.
 */
bool isComparableType(const QSharedPointer<vx::PropertyType>& type);

/**
 * Returns true if the specified property type is poly-numeric (i.e. contains
 * one or more numeric data values).
 *
 * All numeric types are poly-numeric, as are composite types such as Position3D
 * or BoundingBox3D.
 */
bool isPolyNumericType(const vx::PropertyType& type);

using NumberArray = QVarLengthArray<double, 6>;
using NumberExtractor = NumberArray (*)(const QVariant&);

using NumberFunction = std::function<double(double)>;
using NumberManipulator = QVariant (*)(const QVariant&, const NumberFunction&);

/**
 * Returns a function that converts a table cell to a list of numbers contained
 * within it.
 *
 * This can be a single number (e.g. for Float columns) or multiple numbers
 * (e.g. for Position3Ds or BoundingBox3Ds).
 */
NumberExtractor getNumberExtractor(const vx::PropertyType& type);

/**
 * Returns a function that manipulates each component of a polynumeric table
 * value.
 */
NumberManipulator getNumberManipulator(const vx::PropertyType& type);

/**
 * Returns a string suitable for displaying an axis label for a specific table
 * column.
 */
QString getColumnLabel(QSharedPointer<vx::TableData> tableData,
                       QString columnName);
QString getColumnLabel(const vx::TableColumn& column);

}  // namespace TableUtils
