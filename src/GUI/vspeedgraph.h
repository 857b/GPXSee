#ifndef VSPEEDGRAPH_H
#define VSPEEDGRAPH_H

#include <QList>
#include "graphtab.h"

class VSpeedGraphItem;
class Track;

class VSpeedGraph : public GraphTab
{
	Q_OBJECT

public:
	VSpeedGraph(QWidget *parent = 0);
	~VSpeedGraph();

	QString label() const {return tr("Vertical Speed");}
	QList<GraphItem*> loadData(const Data &data);
	void clear();
	void setUnits(Units units);
	void setTimeType(TimeType type);
	void showTracks(bool show);

private:
	GraphItem *loadGraph(const Graph &graph, const Track &track,
	  const QColor &color, bool primary);
	qreal avg() const;
	qreal max() const {return bounds().bottom();}
	qreal min() const {return bounds().top();}
	void setYUnits();
	void setInfo();

	QVector<QPointF> _avg;
	QVector<QPointF> _mavg;

	Units _units;
	TimeType _timeType;

	bool _showTracks;
	QList<VSpeedGraphItem *> _tracks;
};

#endif // VSPEEDGRAPH_H
