#include "elevationgraph.h"

#include <QCoreApplication>
#include "data/data.h"

static const Unit::Fmt fmt = Unit::Fmt(0, false);

ElevationGraphItem_data::ElevationGraphItem_data(const Graph& g)
{
	_ascent = _descent = 0;
	for (int i = 0; i < g.size(); i++) {
		const GraphSegment &segment = g.at(i);

		for (int j = 1; j < segment.size(); j++) {
			qreal cur = segment.at(j).y();
			qreal prev = segment.at(j-1).y();

			if (cur > prev)
				_ascent += cur - prev;
			if (cur < prev)
				_descent += prev - cur;
		}
	}
}

ElevationGraphItem_data::ElevationGraphItem_data(const Track& t, int chId)
{
	_ascent = _descent = 0;
	const QList<Track::Segment>& sgs(t.segments());
	for (int i = 0; i < sgs.size(); ++i) {
		const Track::Segment& sg(sgs.at(i));
		const Track::Channel* ch(sg.findChannel(chId));
		if (!ch) continue;

		for (int j = 1; j < ch->size(); ++j) {
			qreal cur  = ch->at(j);
			qreal prev = ch->at(j-1);
			if (cur > prev)
				_ascent += cur - prev;
			if (cur < prev)
				_descent += prev - cur;
		}
	}
}

void ElevationGraphItem_data::makeTooltip(ToolTip& tt, Unit u,
		                                  qreal min, qreal max) const
{
	tt.insert(tr("Ascent"),  ascent(),  u, fmt);
	tt.insert(tr("Descent"), descent(), u, fmt);
	tt.insert(tr("Maximum"), max,       u, fmt);
	tt.insert(tr("Minimum"), min,       u, fmt);
}


QString ElevationGraphRItem::info() const
{
	ToolTip tt;
	const Unit& u(_units == Metric ? Unit::m : Unit::ft);
	ElevationGraphItem_data::makeTooltip(tt, u,
			GraphItem0::min(), GraphItem0::max());
	return tt.toString();
}


ElevationGraph::ElevationGraph(QWidget *parent)
	: GraphTab1(CTelevation, parent),
	  _routeAscent(0), _routeDescent(0),
	  _showRoutes(true)
{
	setYUnits(units());
	setSliderFmt(fmt);
	setMinYRange(50.0);
}

ElevationGraph::~ElevationGraph()
{
	qDeleteAll(_routes);
}
	
GraphItem1* ElevationGraph::makeTrackItem(GraphSet* set, int chId,
									const GraphItem1::Style& st)
{
	return new ElevationGraphTItem(set, chId, graphType(), st, this);
}

void ElevationGraph::updateInfoKeys()
{
	if (std::isnan(max()) || std::isnan(min()))
		clearInfo();
	else {
		Unit u(yUnit());
		addInfo(tr("Ascent"),  ascent(),  u, fmt);
		addInfo(tr("Descent"), descent(), u, fmt);
		addInfo(tr("Maximum"), max(),     u, fmt);
		addInfo(tr("Minimum"), min(),     u, fmt);
	}
}

ElevationGraphRItem *ElevationGraph::loadRGraph(const Graph &graph,
		const QColor &color, bool primary)
{
	if (!graph.isValid())
		return 0;

	ElevationGraphRItem *gi = new ElevationGraphRItem(
							graph, graphType(), _width,
							color, primary ? Qt::SolidLine : Qt::DashLine);
	gi->setUnits(units());

	_routes.append(gi);
	if (_showRoutes)
		addGraph(gi);

	if (primary) {
		_routeAscent  += gi->ascent();
		_routeDescent += gi->descent();
	}

	return gi;
}

QList<GraphItem*> ElevationGraph::loadData(GData &data)
{
	QList<GraphItem*> graphs = GraphTab1::loadData(data);

	for (int i = 0; i < data.routes().count(); i++) {
		QColor color(_palette.nextColor());
		const GraphPair &gp = data.routes().at(i).elevation();

		ElevationGraphRItem *primary 
					= loadRGraph(gp.primary(), color, true);
		if (!primary) continue;
		graphs[i] = primary;
		ElevationGraphRItem *secondary
					= loadRGraph(gp.secondary(), color, false);
		if (secondary)
			primary->setSecondaryGraph(secondary);
	}

	onGSetChange();

	return graphs;
}

void ElevationGraph::clear()
{
	qDeleteAll(_routes);
	_routes.clear();

	_routeAscent = 0;
	_routeDescent = 0;

	GraphTab1::clear();
}

void ElevationGraph::setYUnits(Units units)
{
	if (units == Metric)
		setYUnit(Unit::m);
	else
		setYUnit(Unit::ft);
}

void ElevationGraph::setUnits(Units units)
{
	setYUnits(units);
	GraphTab1::setUnits(units);
}

void ElevationGraph::showRoutes(bool show)
{
	_showRoutes = show;
	for (int i = 0; i < _routes.size(); i++) {
		if (show)
			addGraph(_routes.at(i));
		else
			removeGraph(_routes.at(i));
	}
	onGSetChange();
}

qreal ElevationGraph::trackAscent() const
{
	return sumTrackItems<ElevationGraphTItem>(&ElevationGraphTItem::ascent);
}

qreal ElevationGraph::trackDescent() const
{
	return sumTrackItems<ElevationGraphTItem>(&ElevationGraphTItem::descent);
}

qreal ElevationGraph::ascent() const
{
	qreal val = 0;

	if (_showRoutes)
		val += _routeAscent;
	if (_showTracks)
		val += trackAscent();

	return val;
}

qreal ElevationGraph::descent() const
{
	qreal val = 0;

	if (_showRoutes)
		val += _routeDescent;
	if (_showTracks)
		val += trackDescent();

	return val;
}
