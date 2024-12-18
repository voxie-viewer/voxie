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

#include "Task.hpp"

vx::Task::Info::Info(
    State state, double progress,
    const QList<std::tuple<QSharedPointer<Task>, double>>& subTasks,
    const QString& displayName, const QList<QSharedPointer<Info>>& subTaskInfos)
    : state_(state),
      progress_(progress),
      subTasks_(subTasks),
      displayName_(displayName),
      subTaskInfos_(subTaskInfos) {
  if (subTasks.size() != subTaskInfos.size())
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Task::Info: subTasks.size() != subTaskInfos.size()");
}
vx::Task::Info::~Info() {}

vx::Task::Task()
    : RefCountedObject("Task"),
      info_(createQSharedPointer<Info>(
          State::NotStarted, 0.0,
          QList<std::tuple<QSharedPointer<Task>, double>>{}, "",
          QList<QSharedPointer<Info>>{})) {}
vx::Task::~Task() {}

void vx::Task::setInfo(QMutexLocker* locker,
                       QSharedPointer<const Info> newInfo) {
  if (locker->mutex() != &mutex)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "locker->mutex() != &mutex");
  if (!newInfo)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError", "!newInfo");

  if (info_ == newInfo) return;

  info_ = newInfo;

  // This will emit this->taskChanged(info_) but will compress multiple
  // invocations of setInfo() while the slots are still running into a
  // single invocation.
  changePending = true;
  if (!changeRunning) {
    enqueueOnThread(this, [this] { emitChanged(); });
    changeRunning = true;
  }
}

void vx::Task::emitChanged() {
  QMutexLocker lock(&mutex);

  if (!changeRunning) {
    qCritical() << "Task::emitChanged: changeRunning is false;";
    return;
  }
  if (!changePending) {
    qCritical() << "Task::emitChanged: changePending is false;";
    changeRunning = false;
    return;
  }
  QSharedPointer<const Info> info = info_;
  changePending = false;

  lock.unlock();
  Q_EMIT this->taskChanged(info);
  lock.relock();

  if (changePending)
    enqueueOnThread(this, [this] { emitChanged(); });
  else
    changeRunning = false;
}

QSharedPointer<const vx::Task::Info> vx::Task::info() {
  QMutexLocker locker(&mutex);
  return info_;
}
vx::Task::State vx::Task::state() {
  QMutexLocker locker(&mutex);
  return info_->state();
}
double vx::Task::progress() {
  QMutexLocker locker(&mutex);
  return info_->progress();
}
QList<std::tuple<QSharedPointer<vx::Task>, double>> vx::Task::subTasks() {
  QMutexLocker locker(&mutex);
  return info_->subTasks();
}
QString vx::Task::displayName() {
  QMutexLocker locker(&mutex);
  return info_->displayName();
}

void vx::Task::setState(State state) {
  QMutexLocker locker(&mutex);
  bool hasSubTasks = info_->subTasks().size() > 0;

  // TODO: Add special cases for Operations (see DBus description of SetState())

  // TODO: Implement subtasks
  State newState = state;
  double progress = info_->progress();
  if (hasSubTasks) {
    qWarning() << "Task::setState: TODO: Implement subtasks";
    return;
  } else {
    if (state == State::NotStarted)
      progress = 0.0;
    else if (state == State::Finished)
      progress = 1.0;
  }

  auto newInfo =
      createQSharedPointer<Info>(newState, progress, info_->subTasks(),
                                 info_->displayName(), info_->subTaskInfos());
  setInfo(&locker, newInfo);
}

void vx::Task::setProgress(double progress) {
  QMutexLocker locker(&mutex);

  // TODO: Should this set the state from Finished to Running? See also DBus
  // documentation for SetProgress().
  State newState = State::Running;

  // Set new state and progress, remove all subTasks.
  auto newInfo = createQSharedPointer<Info>(
      newState, progress, QList<std::tuple<QSharedPointer<Task>, double>>{},
      info_->displayName(), QList<QSharedPointer<Info>>{});
  setInfo(&locker, newInfo);
}

void vx::Task::setSubTasks(
    const QList<std::tuple<QSharedPointer<Task>, double>>& subTasks) {
  // TODO: Implement subtasks
  qWarning() << "Task::setSubTasks: TODO: Implement subtasks";
  Q_UNUSED(subTasks);
}

void vx::Task::setDisplayName(const QString& displayName) {
  QMutexLocker locker(&mutex);

  if (info_->displayName() == displayName)
    // Nothing to do
    return;

  auto newInfo = createQSharedPointer<Info>(info_->state(), info_->progress(),
                                            info_->subTasks(), displayName,
                                            info_->subTaskInfos());
  setInfo(&locker, newInfo);
}
