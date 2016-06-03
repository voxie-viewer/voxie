#include "colorizerworker.hpp"

#include <Voxie/data/colorizer.hpp>

ColorizerWorker::ColorizerWorker(voxie::data::FloatImage sourceImage, voxie::data::Colorizer* colorizer) :
    QObject(),
    sourceImage(sourceImage.clone())
{
    this->colorizer = colorizer->clone();
    this->colorizer->setParent(this);
}

void ColorizerWorker::run() {
    QImage targetImage = sourceImage.toQImage(this->colorizer);

    emit imageColorized(targetImage);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
