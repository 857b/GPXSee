#include "temperaturegraph.h"

static const Unit::Fmt fmt = Unit::Fmt(1);

void TemperatureGraphItem::makeTooltip(ToolTip& tt) const
{
	const Unit&  u = graph().yUnit();
	tt.insert(tr("Average"), avg(), u, fmt);
	tt.insert(tr("Minimum"), min(), u, fmt);
	tt.insert(tr("Maximum"), max(), u, fmt);

}

TemperatureGraph::TemperatureGraph(QWidget *parent)
	: GraphTab2(CTtemperature, parent)
{
	setYUnits(units());
	setSliderFmt(fmt);
}

void TemperatureGraph::updateTracksInfos()
{
	const Unit&  u(yUnit());
	addInfo(tr("Average"), avg(), u, fmt);
	addInfo(tr("Minimum"), min(), u, fmt);
	addInfo(tr("Maximum"), max(), u, fmt);
}

GraphItem1* TemperatureGraph::makeTrackItem(GraphSet* set, int chId,
								const GraphItem1::Style& st)
{
	return new TemperatureGraphItem(set, chId, graphType(), st, this);
}

void TemperatureGraph::setYUnits(Units units)
{
	setYUnit(units == Metric ? Unit::cls : Unit::fhr);
}

void TemperatureGraph::setUnits(Units units)
{
	setYUnits(units);
	GraphView::setUnits(units);
}
