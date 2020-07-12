#include <QLocale>
#include "tooltip.h"
#include "format.h"
#include "vspeedgraphitem.h"


VSpeedGraphItem::VSpeedGraphItem(const Graph &graph,
		GraphType type, int width, const QColor &color,
		Qt::PenStyle style, qreal movingTime, QGraphicsItem *parent)
	: GraphItem(graph, type, width, color, style, parent)
{
	_timeType = Total;

	_max = GraphItem::max();
	qreal deltaY = 0;
	for (int j = 0; j < graph.size(); ++j)
		deltaY += graph.at(j)._sum;
	_avg = deltaY / graph.last().last().t();
	_mavg = deltaY / movingTime;
} 

QString VSpeedGraphItem::info() const
{ 
	ToolTip tt;
	qreal scale = 1.0;
	QString su = tr("m/s");
	QLocale l(QLocale::system());

	tt.insert(tr("Maximum"), l.toString(max() * scale, 'f', 1)
	  + UNIT_SPACE + su);
	tt.insert(tr("Minimum"), l.toString(min() * scale, 'f', 1)
	  + UNIT_SPACE + su);
	tt.insert(tr("Average"),
			l.toString((_timeType == Total) ? avg() * scale : mavg() * scale,
				'f', 2) + UNIT_SPACE + su);

	return tt.toString();
}

void VSpeedGraphItem::setTimeType(TimeType type)
{
	_timeType = type;
}
