#include "speedgraph.h"

#include <cmath>
#include "format.h"

static const Unit::Fmt fmt = Unit::Fmt(1, false);

void SpeedGraphItem::makeTooltip(ToolTip& tt) const
{
	const Unit&  u = graph().yUnit();
	const Unit& pu = static_cast<const SpeedGraph&>(graph()).paceUnit();
	qreal avg  = GraphItem2::avg();
	qreal pace = 1. / avg;

	tt.insert(tr("Maximum"), max(), u, fmt);
	tt.insert(tr("Average"), avg,   u, fmt);
	if (std::isfinite(pace))
		tt.insert(tr("Pace"),
			  pu.format(Format::timeSpan(pu.toUnit(pace), false, true)));
}

SpeedGraph::SpeedGraph(QWidget *parent) : GraphTab2(CTspeed, parent)
{
	setYUnits(units());
	setSliderFmt(fmt);
}

void SpeedGraph::updateTracksInfos()
{
	const Unit&  u(yUnit());
	const Unit& pu(paceUnit());
	qreal avg  = GraphTab2::avg();
	qreal pace = 1. / avg;

	addInfo(tr("Average"), avg,   u, fmt);
	addInfo(tr("Maximum"), max(), u, fmt);
	if (std::isfinite(pace))
		GraphView::addInfo(tr("Pace"),
			pu.format(Format::timeSpan(pu.toUnit(pace), false, true)));
}

GraphItem1* SpeedGraph::makeTrackItem(GraphSet* set, int chId,
								const GraphItem1::Style& st)
{
	return new SpeedGraphItem(set, chId, graphType(), st, this);
}

void SpeedGraph::setYUnits(Units units)
{
	switch(units) {
		case Nautical:
			setYUnit(Unit::kn);
			_paceUnit = Unit::sPnmi;
			break;
		case Imperial:
			setYUnit(Unit::miPh);
			_paceUnit = Unit::sPmi;
			break;
		default:
			setYUnit(Unit::kmPh);
			_paceUnit = Unit::sPkm;
	}
}

void SpeedGraph::setUnits(Units units)
{
	setYUnits(units);
	GraphTab2::setUnits(units);
}
