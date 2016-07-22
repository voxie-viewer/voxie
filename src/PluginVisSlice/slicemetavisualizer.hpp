#pragma once

#include <Voxie/plugin/metavisualizer.hpp>

/**
 * @brief The SliceMetaVisualizer class provides information about the VisSlice plugin.
 * @author Hans Martin Berner
 */

class SliceMetaVisualizer :
        public voxie::plugin::MetaVisualizer
{
    Q_OBJECT
public:
    explicit SliceMetaVisualizer(QObject *parent = 0);
    ~SliceMetaVisualizer();

    static SliceMetaVisualizer* instance();

    virtual voxie::plugin::VisualizerType type() const override
    {
        return voxie::plugin::vt2D;
    }

    virtual voxie::data::Range requiredSliceCount() const override
    {
        return voxie::data::Range(1);
    }

    virtual voxie::data::Range requiredDataSetCount() const override
    {
        return voxie::data::Range(0);
    }

    virtual voxie::visualization::Visualizer *createVisualizer(const QVector<voxie::data::DataSet*> &dataSets, const QVector<voxie::data::Slice*> &slices) override;

    virtual QString name() const override
    {
        return "Slice";
    }
signals:

public slots:
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
