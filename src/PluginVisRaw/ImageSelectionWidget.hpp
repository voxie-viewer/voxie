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

#include <QRunnable>
#include <QTimer>

#include <QtGui/QIcon>

#include <QtWidgets/QPushButton>

class RawVisualizer;

/**
 * @brief The ImageSelectionWidget class consists of three button for changing
 * the current image
 */
class ImageSelectionWidget : public QWidget {
  Q_OBJECT

 public:
  ImageSelectionWidget(QWidget* parent, RawVisualizer* rv);
  ~ImageSelectionWidget() {}

  void backward();

  /**
   * @brief forward displays the next image goes of the last image to be
   * displayed but not yet loaded.
   */
  void forward();

  void changeTimer(int newTimer);

 private:
  /**
   * @brief play starts/stops the slideshow
   */
  void play();
  RawVisualizer* rv;
  QPushButton* forwardsButton;
  QPushButton* backwardsButton;
  QPushButton* playButton;
  QTimer* timer;

  int timerSec = 100;  // in ms

  bool running = false;
};
