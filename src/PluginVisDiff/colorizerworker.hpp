#pragma once

#include <Voxie/data/floatimage.hpp>

#include <QtCore/QRunnable>

#include <QtGui/QImage>

/**
 * Provides a QRunnable implementation to execute the colorizing process of a Colorizer.
 * @author Hans Martin Berner
 */
class ColorizerWorker :
        public QObject,
        public QRunnable
{
    Q_OBJECT
public:
    /**
     * Takes a source image and a colorizer and clones them to make sure no inconsistencies occur during threaded execution.
     * @param sourceImage the image to be colorized
     * @param colorizer the colorizer including the settings
     */
    ColorizerWorker(voxie::data::FloatImage sourceImage, voxie::data::Colorizer* colorizer);
    void run() override;

signals:
    /**
     * Signals that the thread has finished processing the image.
     * @param image the resulting image
     */
    void imageColorized(QImage image);

private:
    voxie::data::FloatImage sourceImage;
    voxie::data::Colorizer* colorizer;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
