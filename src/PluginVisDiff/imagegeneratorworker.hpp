#pragma once

#include <PluginVisDiff/diffsliceimage.hpp>

#include <Voxie/data/slice.hpp>

#include <QtCore/QRunnable>

/**
 * Provides a QRunnable implementation to allow threaded execution of the image generation procedure.
 * @author Hans Martin Berner, Tim Borner
 */

class ImageGeneratorWorker :
        public QObject,
        public QRunnable
{
    Q_OBJECT
public:
    /**
     * A QRunnable object used to generate an image out of given slice configuration. The slice object is not cloned.
     * @param slices the slices including the settings
     * @param sliceAreaFirst the subarea in the first slice to be rendered
     * @param sliceAreaSecond the subarea in the first slice to be rendered
     * @param imageSize the targeted image size of the output image
     * @param interpolation method to be used to achieve the target size
     */
    ImageGeneratorWorker(const QSharedPointer<voxie::data::VoxelData>& data1,
                         const QSharedPointer<voxie::data::VoxelData>& data2,
                         const voxie::data::Plane& plane1,
                         const voxie::data::Plane& plane2,
                         const QRectF &sliceAreaFirst,
                         const QRectF &sliceAreaSecond,
                         const QSize &imageSize,
                         voxie::data::InterpolationMethod interpolation);

    ~ImageGeneratorWorker();

    void run() override;

signals:
    /**
     * Signals that the run method has finished generating the image.
     * @param si represents the generated images
     */
    void imageGenerated(DiffSliceImage *dsi);

private:
    QSharedPointer<voxie::data::VoxelData> data1;
    QSharedPointer<voxie::data::VoxelData> data2;
    voxie::data::Plane plane1;
    voxie::data::Plane plane2;
    QRectF sliceAreaFirst;
    QRectF sliceAreaSecond;
    QSize imageSize;
    voxie::data::InterpolationMethod interpolation;
    //static volatile int started;
    //static volatile int finished;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
