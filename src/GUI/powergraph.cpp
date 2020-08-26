#include "powergraph.h"

static const Unit::Fmt fmt = Unit::Fmt(1);

PowerGraph::PowerGraph(QWidget *parent)
	: GraphTab2(CTpower, parent)
{
	setYUnit(Unit::w);
	setSliderFmt(fmt);
}

void PowerGraph::updateTracksInfos()
{
	const Unit&  u(yUnit());
	addInfo(tr("Average"), avg(), u, fmt);
	addInfo(tr("Maximum"), max(), u, fmt);
}

GraphItem1* PowerGraph::makeTrackItem(GraphSet* set, int chId,
								const GraphItem1::Style& st)
{
	return new PowerGraphItem(set, chId, graphType(), st, this);
}

void PowerGraphItem::makeTooltip(ToolTip& tt) const
{
	const Unit&  u = graph().yUnit();
	tt.insert(tr("Maximum"), max(), u, fmt);
	tt.insert(tr("Average"), avg(), u, fmt);
}
