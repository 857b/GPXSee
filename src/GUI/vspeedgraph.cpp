#include <QLocale>
#include "data/data.h"
#include "tooltip.h"
#include "format.h"
#include "vspeedgraphitem.h"
#include "vspeedgraph.h"


VSpeedGraph::VSpeedGraph(QWidget *parent) : GraphTab(parent)
{
	_units = Metric;
	_timeType = Total;
	_showTracks = true;

	setYUnits();
	setYLabel(tr("Vertical Speed"));

	setSliderPrecision(1);
}

VSpeedGraph::~VSpeedGraph()
{
	qDeleteAll(_tracks);
}

void VSpeedGraph::setInfo()
{
	if (_showTracks) {
		QLocale l(QLocale::system());

		GraphView::addInfo(tr("Average"), l.toString(avg() * yScale(), 'f',
		  1) + UNIT_SPACE + yUnits());
		GraphView::addInfo(tr("Maximum"), l.toString(max() * yScale(), 'f',
		  1) + UNIT_SPACE + yUnits());
	} else
		clearInfo();
}

GraphItem *VSpeedGraph::loadGraph(const Graph &graph, const Track &track,
  const QColor &color, bool primary)
{
	if (!graph.isValid())
		return 0;

	VSpeedGraphItem *gi = new VSpeedGraphItem(graph, _graphType, _width,
	  color, primary ? Qt::SolidLine : Qt::DashLine, track.movingTime());
	gi->setTimeType(_timeType);
	gi->setUnits(_units);

	_tracks.append(gi);
	if (_showTracks)
		addGraph(gi);

	if (primary) {
		_avg.append(QPointF(track.distance(), gi->avg()));
		_mavg.append(QPointF(track.distance(), gi->mavg()));
	}

	return gi;
}

QList<GraphItem*> VSpeedGraph::loadData(const Data &data)
{
	QList<GraphItem*> graphs;

	for (int i = 0; i < data.tracks().count(); i++) {
		QColor color(_palette.nextColor());
		const Track &track = data.tracks().at(i);
		const Graph &graph = track.vspeed();

		graphs.append(loadGraph(graph, track, color, true));
	}

	for (int i = 0; i < data.routes().count(); i++) {
		_palette.nextColor();
		graphs.append(0);
	}

	for (int i = 0; i < data.areas().count(); i++)
		_palette.nextColor();

	setInfo();
	redraw();

	return graphs;
}

qreal VSpeedGraph::avg() const
{
	qreal sum = 0, w = 0;
	const QVector<QPointF> &vector = (_timeType == Moving) ? _mavg : _avg;

	for (int i = 0; i < vector.size(); i++) {
		const QPointF &p = vector.at(i);
		sum += p.y() * p.x();
		w += p.x();
	}

	return (sum / w);
}

void VSpeedGraph::clear()
{
	qDeleteAll(_tracks);
	_tracks.clear();

	_avg.clear();
	_mavg.clear();

	GraphTab::clear();
}

void VSpeedGraph::setYUnits()
{
	GraphView::setYUnits(tr("m/s"));
	setYScale(1.0);
}

void VSpeedGraph::setUnits(Units units)
{
	_units = units;

	setYUnits();
	setInfo();

	GraphView::setUnits(units);
}

void VSpeedGraph::setTimeType(enum TimeType type)
{
	_timeType = type;

	for (int i = 0; i < _tracks.size(); i++)
		_tracks.at(i)->setTimeType(type);

	setInfo();
	redraw();
}

void VSpeedGraph::showTracks(bool show)
{
	_showTracks = show;

	for (int i = 0; i < _tracks.size(); i++) {
		if (show)
			addGraph(_tracks.at(i));
		else
			removeGraph(_tracks.at(i));
	}

	setInfo();

	redraw();
}
