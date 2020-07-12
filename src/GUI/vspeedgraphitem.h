#ifndef VSPEEDGRAPHITEM_H
#define VSPEEDGRAPHITEM_H

#include "timetype.h"
#include "graphitem.h"

class VSpeedGraphItem : public GraphItem
{
	Q_OBJECT

public:
	VSpeedGraphItem(const Graph &graph,
			GraphType type, int width, const QColor &color,
			Qt::PenStyle style, qreal movingTime,
			QGraphicsItem *parent = 0);

	qreal avg() const {return _avg;}
	qreal mavg() const {return _mavg;}
	qreal max() const {return _max;}

	QString info() const;

	void setTimeType(TimeType type);

private:
	qreal _avg, _mavg, _max;
	TimeType _timeType;
};

#endif // VSPEEDGRAPHITEM_H
