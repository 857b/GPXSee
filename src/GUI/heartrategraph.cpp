#include "heartrategraph.h"

static const Unit::Fmt fmt = Unit::Fmt(0);

void HeartRateGraphItem::makeTooltip(ToolTip& tt) const
{
	const Unit& u = graph().yUnit();
	tt.insert(tr("Maximum"), max(), u, fmt);
	tt.insert(tr("Average"), avg(), u, fmt);
}

HeartRateGraph::HeartRateGraph(QWidget *parent)
	: GraphTab2(CTheartRate, parent)
{
	GraphView::setYUnit(Unit::bpm);
	setSliderFmt(fmt);
}

void HeartRateGraph::updateTracksInfos()
{
	const Unit& u = yUnit();
	addInfo(tr("Average"), avg(), u, fmt);
	addInfo(tr("Maximum"), max(), u, fmt);
}

GraphItem1* HeartRateGraph::makeTrackItem(GraphSet* set, int chId,
								const GraphItem1::Style& st)
{ 
	return new HeartRateGraphItem(set, chId, graphType(), st, this);
}
