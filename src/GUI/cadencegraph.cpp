#include "cadencegraph.h"

static const Unit::Fmt fmt = Unit::Fmt(1);

CadenceGraph::CadenceGraph(QWidget *parent)
	: GraphTab2(CTcadence, parent)
{
	setYUnit(Unit::rpm);
	setSliderFmt(fmt);
}

void CadenceGraph::updateTracksInfos()
{
	const Unit& u(yUnit());
	addInfo(tr("Average"), avg(), u, fmt);
	addInfo(tr("Maximum"), max(), u, fmt);
}

GraphItem1* CadenceGraph::makeTrackItem(GraphSet* set, int chId,
								const GraphItem1::Style& st)
{
	return new CadenceGraphItem(set, chId, graphType(), st, this);
}

void CadenceGraphItem::makeTooltip(ToolTip& tt) const
{
	const Unit&  u = graph().yUnit();
	tt.insert(tr("Maximum"), max(), u, fmt);
	tt.insert(tr("Average"), avg(), u, fmt);
}
