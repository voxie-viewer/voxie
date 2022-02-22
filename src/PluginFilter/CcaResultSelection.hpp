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

#include <PluginFilter/Prototypes.forward.hpp>

#include <Voxie/Data/LabelConstraint.hpp>
#include <Voxie/Data/PropertySection.hpp>
#include <Voxie/Data/TableNode.hpp>
#include <Voxie/Node/FilterNode.hpp>
#include <Voxie/Node/NodePrototype.hpp>

#include <Voxie/IO/RunFilterOperation.hpp>

#include <QList>

namespace vx {

// TODO: Expose filter as property

namespace filters {
// TODO: Fully rename to TableFilter
class TableFilter : public FilterNode {
  NODE_PROTOTYPE_DECL(TableFilter)

 public:
  TableFilter();

  virtual QVariant getNodePropertyCustom(QString key) override;
  virtual void setNodePropertyCustom(QString key, QVariant value) override;

 private:
  int idCounter;
  TableNode* selectedCcaResults;

  /**
   * @brief calculate selects the conencted LabelNodes by the given
   * constraints.
   * no python script is executed
   */
  QSharedPointer<vx::io::RunFilterOperation> calculate() override;

  /**
   * @brief createConstraint creates a new constraint, adds it to the list
   * constraints and adds it to the layout to display the constraint.
   */
  void createConstraint();

  /**
   * @brief removeConstraint removes the constraint with the @param id from the
   * list constraints
   */
  void removeConstraint(int id);

  /**
   * @brief checkConstraint checks, if the specified constraint is true or not
   * @param currentValue defines the value of the label
   * @param op defines the operation that is used by the selection
   * @param limit is the limit for currentvalue
   * @return true if the constraint is true, else it returns false.
   */
  bool checkConstraint(int currentValue, QString op, double limit);

  // QPushButton* button;
  PropertySection* selectProperties;
  QList<LabelConstraint*> constraints;
};

// TODO: Fully rename to TableFilter
typedef TableFilter CcaResultSelection;
typedef TableFilter CCAResultSelection;
}  // namespace filters
}  // namespace vx
