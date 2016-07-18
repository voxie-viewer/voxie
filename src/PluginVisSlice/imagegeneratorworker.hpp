#pragma once

#include <Voxie/data/slice.hpp>

#include <QtCore/QRunnable>

/**
 * Provides a QRunnable implementation to allow threaded execution of the image generation procedure.
 * @author Hans Martin Berner
 */

class ImageGeneratorWorker :
		public QObject,
		public QRunnable
{
	Q_OBJECT
public:
	/**
	 * A QRunnable object used to generate an image out of given slice configuration. The slice object is not cloned.
	 * @param slice the slice including the settings
	 * @param sliceArea the subarea in the slice to be rendered
	 * @param imageSize the targeted image size of the output image
	 * @param interpolation method to be used to achieve the target size
	 */
	ImageGeneratorWorker(const QSharedPointer<voxie::data::VoxelData>& data,
                         const voxie::data::Plane& plane,
						 const QRectF &sliceArea,
						 const QSize &imageSize,
						 voxie::data::InterpolationMethod interpolation);

	~ImageGeneratorWorker();

	void run() override;

signals:
	/**
	 * Signals that the run method has finished generating the image.
	 * @param si represents the generated image
	 */
    void imageGenerated(voxie::data::SliceImage si);

private:
    QSharedPointer<voxie::data::VoxelData> data;
	voxie::data::Plane plane;
	QRectF sliceArea;
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
