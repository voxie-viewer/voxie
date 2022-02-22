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

#include <Voxie/Node/FilterNode.hpp>
#include <Voxie/Node/NodePrototype.hpp>
#include <VoxieBackend/IO/Operation.hpp>

#include <QtCore/QList>
#include <QtCore/QSharedPointer>

#include <Voxie/IO/RunFilterOperation.hpp>

namespace vx {
namespace io {

class RunMultipleFilterOperationBase : public vx::io::Operation {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(RunMultipleFilterOperationBase)

 public:
  RunMultipleFilterOperationBase();
  ~RunMultipleFilterOperationBase();
  void filterFinished();

 protected:
  void runFilters(QList<vx::FilterNode*> filterList, bool skipUnchanged);

 private:
  int totalFilters = 0;
  int filterFinishedCount = 0;
};

class RunFilterNode : public QObject {
  Q_OBJECT

 public:
  RunFilterNode(vx::FilterNode* filter) { this->filter = filter; }
  ~RunFilterNode() {}

  QSharedPointer<bool> isFailed = createQSharedPointer<bool>(false);
  bool isFilterFinished = false;
  bool isFilterCancelled = false;
  vx::FilterNode* filter;
  int numParents = 0;
  QSharedPointer<RunMultipleFilterOperationBase> operation;
  QSharedPointer<RunFilterNode> rootNode;
  QList<QSharedPointer<RunFilterNode>> children;

  int numParentsProcessed = 0;

  void clearChildren() {
    for (QSharedPointer<RunFilterNode> child : children) {
      child->clearChildren();
    }
    clear();
  }

  /**
   * @brief Start this filter.
   *
   * @param parentHasChanged Should be set to true if the parent of this filter
   * has changed. This will force the filter to be re-run even if the filter
   * itself hasn't changed. The filter will always be re-run when it has
   * changed.
   */
  void startFilterIfNecessary(bool parentHasChanged) {
    if (!parentHasChanged && !filter->needsRecalculation()) {
      operation->filterFinished();
      this->isFilterFinished = true;
      startChildren(false);
      clear();
      return;
    }

    QSharedPointer<RunFilterOperation> filterOp = filter->run();

    // when this filter is finished call operationFinished()
    filterOp->onFinished(
        this, [this](const QSharedPointer<Operation::ResultError>& error) {
          this->operationFinished(error);
        });
    // TODO: This probably should not connect to QObject::destroyed but to some
    // Operation-specific signal
    connect(filterOp.data(), &QObject::destroyed, this,
            &RunFilterNode::operationStopped);
    connect(operation.data(), &Operation::cancelled, filterOp.data(),
            &Operation::cancel);
  }

  void startChildren(bool parentHasChanged) {
    for (QSharedPointer<RunFilterNode> child : children) {
      child->numParentsProcessed++;
      if (child->numParents == child->numParentsProcessed) {
        child->startFilterIfNecessary(parentHasChanged);
      }
    }
  }

 public Q_SLOTS:
  void operationFinished(const QSharedPointer<Operation::ResultError>& error) {
    isFilterFinished = true;
    // TODO: clean up
    if (error) {
      isFilterCancelled = true;
      *isFailed = true;
      *rootNode->isFailed = true;
      clearChildren();
    }

    // if the filter finished successfully run the child filters afterwards
    if (!isFilterCancelled) {
      startChildren(true);
    }
    clear();
  }

  void operationStopped() {
    isFilterCancelled = true;
    // dont clear if QObject::destroyed emitted after
    // RunFilterOperation::finished
    if (!isFilterFinished) {
      // TODO: Is this ever reached? It should not be because
      // operationFinished() should always be called before this
      qWarning() << "RunMultipleFilterOperationBase::operationStopped() called "
                    "without "
                    "operationFinished";
      *isFailed = true;
      *rootNode->isFailed = true;
      clearChildren();
    }
  }

  void clear() {
    if (!operation.isNull()) {
      operation->filterFinished();
    }
    operation.clear();
    // clear reference to allow delete, once all rootNodes are cleared,
    // QSharedPointer will delete everything else
    rootNode.clear();
  }
};

class FilterGraph {
 public:
  FilterGraph(
      QSharedPointer<RunMultipleFilterOperationBase> runMultipleFilterOperation,
      QList<vx::FilterNode*> filterList) {
    this->root = QSharedPointer<RunFilterNode>(new RunFilterNode(nullptr));
    this->operation = runMultipleFilterOperation;
    for (vx::FilterNode* filter : filterList) {
      // add top level filter to root
      if (!hasFilterParents(filter, filterList)) {
        // dont add filters which are not allowed to run (e.g. filters w/o
        // parent Nodes)
        if (canFilterRun(filter)) {
          QSharedPointer<RunFilterNode> node(new RunFilterNode(filter));
          node->rootNode = root;
          node->operation = operation;
          root->children.append(node);
        }
      }
    }
    // add rest of graph recursively
    for (QSharedPointer<RunFilterNode> node : root->children) {
      addChildren(node, node->filter, filterList);
    }

    // TODO: This probably should not be use QObject::destroyed
    // the operation is finished when the root object is destroyed
    // this means we have to call RunFilterNode::clear() when a filter is done
    // so that the objects get destroyed and the operation can finish
    QObject::connect(
        root.data(), &QObject::destroyed, operation.data(),
        [isFailed = root->isFailed, operation = operation]() {
          if (!operation->isFinished()) {
            if (*isFailed)
              operation->finish(createQSharedPointer<Operation::ResultError>(
                  createQSharedPointer<Exception>(
                      "de.uni_stuttgart.Voxie.RunAllFilterFailed",
                      "One or more filters failed or were cancelled")));
            else
              operation->finish(
                  createQSharedPointer<Operation::ResultSuccess>());
          }
        });
  }

  /**
   * @brief Run the filters in the graph.
   *
   * @param forceRerunAll If set to true all filters in the graph will be run.
   * If set to false only filters which need updating (they themselves or their
   * parents changed) will be run.
   */
  void runFilters(bool forceRerunAll) {
    for (QSharedPointer<RunFilterNode> node : root->children) {
      node->startFilterIfNecessary(forceRerunAll);
    }
  }

 private:
  QSharedPointer<RunFilterNode> root;
  QSharedPointer<RunMultipleFilterOperationBase> operation;

  bool canFilterRun(vx::FilterNode* filter) {
    if (filter->prototype()->allowedInputTypes().empty()) {
      return true;
    }
    for (QSharedPointer<NodePrototype> allowedType :
         filter->prototype()->allowedInputTypes()) {
      for (vx::Node* parent : filter->parentNodes()) {
        if (parent->prototype() == allowedType) {
          return true;
        }
      }
    }
    return false;
  }

  void addChildren(QSharedPointer<RunFilterNode> nodeToAddTo,
                   FilterNode* parentFilter,
                   QList<vx::FilterNode*> filterList) {
    QList<vx::Node*> nodeList;
    QList<vx::FilterNode*> childFilters;
    nodeList.append(parentFilter->childNodes());
    while (nodeList.size() != 0) {
      vx::Node* obj = nodeList.takeLast();
      if (obj->nodeKind() == NodeKind::Filter) {
        childFilters.append(dynamic_cast<vx::FilterNode*>(obj));
      } else {
        nodeList.append(obj->childNodes());
      }
    }
    for (vx::FilterNode* childFilter : childFilters) {
      QSharedPointer<RunFilterNode> child = getOrCreateNode(childFilter);
      // only add the child to the graph if it is part of filterList, because
      // only then it should be run
      if (filterList.contains(childFilter)) {
        nodeToAddTo->children.append(child);
        child->numParents++;
      }
      // check if the children of the child may need to be added to the graph
      addChildren(nodeToAddTo, childFilter, filterList);
    }
  }

  QSharedPointer<RunFilterNode> getOrCreateNode(vx::FilterNode* filterNode) {
    QSharedPointer<RunFilterNode> node = getNodeInChildren(filterNode, root);
    if (node.isNull()) {
      node = QSharedPointer<RunFilterNode>(new RunFilterNode(filterNode));
      node->rootNode = root;
      node->operation = operation;
    }
    return node;
  }

  QSharedPointer<RunFilterNode> getNodeInChildren(
      vx::FilterNode* filterNode, QSharedPointer<RunFilterNode> currentNode) {
    QSharedPointer<RunFilterNode> node;
    for (QSharedPointer<RunFilterNode> child : currentNode->children) {
      if (child->filter == filterNode) {
        return child;
      }
      node = getNodeInChildren(filterNode, child);
      if (!node.isNull()) {
        return node;
      }
    }
    return node;
  }

  bool hasFilterParents(vx::FilterNode* filter,
                        QList<vx::FilterNode*> filterList) {
    QList<vx::Node*> nodeList;
    nodeList.append(filter->parentNodes());
    // depth-first search trough filter's parent-nodes and parent-nodes'
    // parents if there are any filter nodes that come before this one
    while (nodeList.size() != 0) {
      vx::Node* obj = nodeList.takeLast();
      // if we have found a filter node that is an (indirect) parent of the
      // filter, check if it is even part of the filter list that was
      // passed to the operation. If it isn't just keep searching...
      if (obj->nodeKind() == NodeKind::Filter &&
          filterList.contains(dynamic_cast<vx::FilterNode*>(obj))) {
        return true;
      } else {
        nodeList.append(obj->parentNodes());
      }
    }
    return false;
  }
};

}  // namespace io
}  // namespace vx
