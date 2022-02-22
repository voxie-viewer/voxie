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

#include "HelpPageSource.hpp"

#include <QtCore/QMap>

#include <Main/Help/HelpPageSourceAll.hpp>
#include <Main/Help/HelpPageSourceDBusInterface.hpp>
#include <Main/Help/HelpPageSourceIndex.hpp>
#include <Main/Help/HelpPageSourcePrototype.hpp>
#include <Main/Help/HelpPageSourceTopic.hpp>

#include <Voxie/Component/HelpCommon.hpp>

using namespace vx::help;

HelpPageSource::HelpPageSource(const QString& prefix) : prefix_(prefix) {}
HelpPageSource::~HelpPageSource() {}

QString HelpPageSource::urlPrefix() {
  return Protocol + ":" + vx::help::commands::Help + prefix();
}

QMap<QString, QSharedPointer<HelpPageSource>> vx::help::getHelpPageSources(
    const QSharedPointer<HelpPageRegistry>& registry) {
  QMap<QString, QSharedPointer<HelpPageSource>> sources;
  for (const auto& entry : {
           getHelpPageSourcePrototype(registry),
           getHelpPageSourceDBusInterface(),
           getHelpPageSourceTopic(),
           getHelpPageSourceIndex(registry),
           getHelpPageSourceAll(registry),
       })
    sources.insert(entry->prefix(), entry);
  return sources;
}
