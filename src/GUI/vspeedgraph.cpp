#include "vspeedgraph.h"

static const Unit::Fmt fmt_a = Unit::Fmt(2, true);
static const Unit::Fmt fmt_b = Unit::Fmt(1, true);

void VSpeedGraphItem::makeTooltip(ToolTip& tt) const
{
	const Unit& u = graph().yUnit();
	tt.insert(tr("Maximum"), max(), u, fmt_b);
	tt.insert(tr("Minimum"), min(), u, fmt_b);
	tt.insert(tr("Average"), avg(), u, fmt_a);
}

VSpeedGraph::VSpeedGraph(QWidget *parent)
	: GraphTab2(CTvSpeed, parent)
{ 
	setYUnit(Unit::mPs);

	showZero();
	setSliderFmt(Unit::Fmt(1, true));
}

void VSpeedGraph::updateTracksInfos()
{
	const Unit& u = yUnit();
	addInfo(tr("Maximum"), max(), u, fmt_b);
	addInfo(tr("Minimum"), min(), u, fmt_b);
	addInfo(tr("Average"), avg(), u, fmt_a);
}

GraphItem1* VSpeedGraph::makeTrackItem(GraphSet* set, int chId,
								const GraphItem1::Style& st)
{ 
	return new VSpeedGraphItem(set, chId, graphType(), st, this);
}
