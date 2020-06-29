#include <QLocale>
#include "tooltip.h"
#include "format.h"
#include "vspeedgraphitem.h"


VSpeedGraphItem::VSpeedGraphItem(const Graph &graph, GraphType type, int width,
  const QColor &color, Qt::PenStyle style, qreal movingTime,
  QGraphicsItem *parent) : GraphItem(graph, type, width, color, style, parent)
{
	_timeType = Total;

	_max = GraphItem::max();
	_avg = graph.last().last().s() / graph.last().last().t();
	_mavg = graph.last().last().s() / movingTime;
} 

QString VSpeedGraphItem::info() const
{ 
	ToolTip tt;
	qreal scale = 1.0;
	QString su = tr("m/s");
	QLocale l(QLocale::system());

	tt.insert(tr("Maximum"), l.toString(max() * scale, 'f', 1)
	  + UNIT_SPACE + su);
	tt.insert(tr("Average"), l.toString((_timeType == Total)
	  ? avg() * scale : mavg() * scale, 'f', 1) + UNIT_SPACE + su);

	return tt.toString();
}

void VSpeedGraphItem::setTimeType(TimeType type)
{
	_timeType = type;
}
