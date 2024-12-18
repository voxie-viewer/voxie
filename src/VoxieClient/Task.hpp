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

#include <VoxieClient/VoxieClient.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QSharedPointer>

namespace vx {

class VOXIECLIENT_EXPORT Task : public vx::RefCountedObject {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 public:
  enum class State : uint8_t {
    NotStarted,
    Running,
    Finished,
  };

  // Snapshot of the Task's current state. (read-only)
  class Info {
    State state_;
    double progress_;
    QList<std::tuple<QSharedPointer<Task>, double>> subTasks_;
    QString displayName_;
    QList<QSharedPointer<Info>> subTaskInfos_;

   public:
    Info(State state, double progress,
         const QList<std::tuple<QSharedPointer<Task>, double>>& subTasks,
         const QString& displayName,
         const QList<QSharedPointer<Info>>& subTaskInfos);
    ~Info();

    State state() const { return state_; }
    double progress() const { return progress_; }
    const QList<std::tuple<QSharedPointer<Task>, double>>& subTasks() const {
      return subTasks_;
    }
    const QString& displayName() const { return displayName_; }
    const QList<QSharedPointer<Info>>& subTaskInfos() const {
      return subTaskInfos_;
    }
  };

 private:
  QMutex mutex;
  QSharedPointer<const Info> info_;
  bool finishAutomatically_ = false;
  bool changingSubTasks_ = false;

  // Must be called with mutex held
  void setInfo(QMutexLocker* locker, QSharedPointer<const Info> newInfo);

  bool changeRunning = false;
  bool changePending = false;
  void emitChanged();

  // Will set changingSubTasks_ to true for the duration of the guard
  class ChangingSubTasksGuard {
    Task* task;

   public:
    explicit ChangingSubTasksGuard(Task* task);
    ~ChangingSubTasksGuard();

    ChangingSubTasksGuard(const ChangingSubTasksGuard&) = delete;
    ChangingSubTasksGuard& operator=(const ChangingSubTasksGuard&) = delete;
  };

  // Throws if this has task as a direct or indirect child or is equal to task.
  // Also throws if any child has changingSubTasks_ set to true.
  void checkDoesNotHaveChild(Task* task);

 public:
  Task();
  ~Task() override;

  // info(), state(), progress(), subTasks() and displayName() are thread-safe
  // and atomic (i.e. they return a consistent state of the task at one point in
  // time).
  QSharedPointer<const Info> info();
  State state();
  double progress();
  QList<std::tuple<QSharedPointer<Task>, double>> subTasks();
  QString displayName();
  // TODO: Add a human-readable string describing the current state of the task?

  // setState, setProgress, setSubTasks and setDisplayName are thread-safe.
  void setState(State state);
  void setProgress(double progress);
  void setSubTasks(
      const QList<std::tuple<QSharedPointer<Task>, double>>& subTasks);
  void setDisplayName(const QString& displayName);

 Q_SIGNALS:
  // Emitted whenever the task changes.
  void taskChanged(const QSharedPointer<const Info>& info);
};
}  // namespace vx
