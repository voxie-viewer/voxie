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

#include "RunParallel.hpp"

#include <VoxieClient/Exception.hpp>
#include <VoxieClient/Task.hpp>

// TODO: Move Operation / CancellationToken to VoxieClient
#include <VoxieBackend/IO/Operation.hpp>

Q_NORETURN void vx::Intern::raiseRunParallelInternalError(const char* info) {
  throw vx::Exception("de.uni_stuttgart.Voxie.InternalError", info);
}

void vx::Intern::runParallelSetProgress(Task* task, double progress) {
  if (task) task->setProgress(progress);
}

void vx::Intern::runParallelSetFinished(Task* task) {
  if (task) task->setState(Task::State::Finished);
}

void vx::Intern::runParallelThrowIfCancelled(
    CancellationToken* cancellationToken) {
  // if (cancellationToken) cancellationToken->throwIfCancelled();

  // TODO: Hack to avoid needing to link things in VoxieBackend
  if (cancellationToken && cancellationToken->cancelledVal.load() != 0)
    throw OperationCancelledException();
}
