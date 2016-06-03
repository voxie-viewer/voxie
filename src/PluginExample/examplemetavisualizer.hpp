#pragma once

#include <Voxie/plugin/metavisualizer.hpp>

class ExampleMetaVisualizer :
		public voxie::plugin::MetaVisualizer
{
	Q_OBJECT
public:
	explicit ExampleMetaVisualizer(QObject *parent = 0);
	~ExampleMetaVisualizer();

	virtual voxie::plugin::VisualizerType type() const override
	{
		return voxie::plugin::vtMiscellaneous;
	}

	virtual voxie::data::Range requiredSliceCount() const override
	{
		return voxie::data::Range(0);
	}

	virtual voxie::data::Range requiredDataSetCount() const override
	{
		return voxie::data::Range(0);
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
