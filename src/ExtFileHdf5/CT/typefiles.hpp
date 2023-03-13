
#include <Math/Vector3.hpp>
#include <Math/DiagMatrix3.hpp>
#include <Math/Float.hpp>

#include <HDF5/Matlab.hpp>
#include <HDF5/MatlabVector2.hpp>
#include <HDF5/MatlabVector3.hpp>
#include <HDF5/MatlabDiagMatrix3.hpp>
#include <HDF5/DelayedArray.hpp>
#include <HDF5/Array.hpp>

#include <boost/scoped_ptr.hpp>

#include <QVector>


template <typename T, bool delayLoading = false>
struct TypeCheckGen;

template <typename T, bool delayLoading>
struct TypeCheckGen {
    boost::optional<std::string> Type;

#define MEMBERS(m)   \
    m (Type)
    HDF5_MATLAB_DECLARE_TYPE (TypeCheckGen, MEMBERS)
#undef MEMBERS
};



template <typename T>
std::shared_ptr<TypeCheckGen<T,true> > loadTo (const TypeCheckGen<T, true>& self) {
    std::shared_ptr<TypeCheckGen<T,true> > data = std::make_shared<TypeCheckGen<T,true> > ();
    data->Type = self.Type;
    return data;
}



