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

#include "SpaceNavClient.hpp"

#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <QtCore/QSharedPointer>

using namespace vx::spnav;

SpaceNavClient::Private::~Private() {}

SpaceNavClient::SpaceNavClient() : QObject() {
  // qDebug() << "SpaceNavClient()";

  if (!priv) priv.reset(createPrivateX11());
  if (priv && !priv->isOk()) priv.reset();

  if (!priv) priv.reset(createPrivateUnix());
  if (priv && !priv->isOk()) priv.reset();

  // TODO: add implementation for windows
}
SpaceNavClient::~SpaceNavClient() {
  // qDebug() << "~SpaceNavClient()";
}

QSharedPointer<SpaceNavClient> SpaceNavClient::getClient() {
  static QWeakPointer<SpaceNavClient> weak;
  static QMutex weakMutex;

  QMutexLocker locker(&weakMutex);

  QSharedPointer<SpaceNavClient> strong = weak;
  if (strong) return strong;

  strong.reset(new SpaceNavClient(),
               [](SpaceNavClient* obj) { obj->deleteLater(); });
  weak = strong;
  return strong;
}
