#pragma once

#include <Voxie/data/slice.hpp>

#include <QtCore/QObject>

/**
 * @brief The DiffSliceImage class holds the generated SliceImage of two SliceImages and
 *        its diffImage as Float Image
 * @author Tim Borner
 */
class DiffSliceImage {
public:
    DiffSliceImage()
    {

    }

    voxie::data::SliceImage first;
    voxie::data::SliceImage second;
    voxie::data::FloatImage diffImage;
};

Q_DECLARE_METATYPE(DiffSliceImage)

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
