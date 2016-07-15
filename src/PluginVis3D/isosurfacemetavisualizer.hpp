#pragma once

#include <Voxie/plugin/metavisualizer.hpp>

class IsosurfaceMetaVisualizer :
        public voxie::plugin::MetaVisualizer
{
    Q_OBJECT
public:
    explicit IsosurfaceMetaVisualizer(QWidget *parent = 0);

    static IsosurfaceMetaVisualizer* instance();

    virtual voxie::plugin::VisualizerType type() const
    {
        return voxie::plugin::vt3D;
    }

    virtual voxie::data::Range requiredSliceCount() const override
    {
        return voxie::data::Range(0);
    }

    virtual voxie::data::Range requiredDataSetCount() const override
    {
        return voxie::data::Range(1);
    }

    virtual QString name() const
    {
        return "Isosurface";
    }

    virtual voxie::visualization::Visualizer *createVisualizer(const QVector<voxie::data::DataSet*> &dataSets, const QVector<voxie::data::Slice*> &slices) override;

signals:

public slots:

};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
