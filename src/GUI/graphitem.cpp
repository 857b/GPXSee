#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include "popup.h"
#include "graphitem.h"


GraphItem::GraphItem(QObject* oParent, GraphType type,
			int width, const QColor &color, Qt::PenStyle style,
			QGraphicsItem *iParent)
	: QObject(oParent), GraphicsItem(iParent),
	  _type(type),
	  _shpWidth(4)
{
	_units = Metric;
	_pen = QPen(color, width, style);
	_pen.setCosmetic(true);
	_pen.setWidth(width);
	setZValue(2.0);
	setAcceptHoverEvents(true);
}

void GraphItem::updateG()
{
	prepareGeometryChange();
	updatePath();
	updateShape();
	updateBounds();
}

void GraphItem::updateShape()
{
	QPainterPathStroker s;
	s.setWidth(qMax(_shpWidth, _pen.widthF()));
	_shape = s.createStroke(_path);
}

void GraphItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
  QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	painter->setPen(_pen);
	painter->drawPath(_path);

/*
	QPen p = QPen(QBrush(Qt::red), 0);
	painter->setPen(p);
	painter->drawRect(boundingRect());
*/
}

void GraphItem::setGraphType(GraphType type)
{
	if (type == _type)
		return;

	_type = type;
	updateG();
}

void GraphItem::setColor(const QColor &color)
{
	if (_pen.color() == color)
		return;

	_pen.setColor(color);
	update();
}

void GraphItem::setWidth(int width)
{
	if (width == _pen.width())
		return;

	prepareGeometryChange();

	_pen.setWidth(width);

	updateShape();
}

void GraphItem::emitSliderPositionChanged(qreal pos)
{
	if (_type == Time)
		emit sliderPositionChanged(hasTime() ? distanceAtTime(pos) : NAN);
	else
		emit sliderPositionChanged(pos);
}

void GraphItem::hover(bool hover)
{
	if (hover) {
		_pen.setWidth(_pen.width() + 1);
		setZValue(zValue() + 1.0);
	} else {
		_pen.setWidth(_pen.width() - 1);
		setZValue(zValue() - 1.0);
	}

	update();
}

void GraphItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	Q_UNUSED(event);

	_pen.setWidth(_pen.width() + 1);
	setZValue(zValue() + 1.0);
	update();

	emit selected(true);
}

void GraphItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	Q_UNUSED(event);

	_pen.setWidth(_pen.width() - 1);
	setZValue(zValue() - 1.0);
	update();

	emit selected(false);
}

void GraphItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	Popup::show(event->screenPos(), info(), event->widget());
	GraphicsItem::mousePressEvent(event);
}


// GraphItem0

GraphItem0::GraphItem0(const Graph &graph, GraphType type, int width,
	  const QColor &color, Qt::PenStyle style, QGraphicsItem *parent)
  : GraphItem(NULL, type, width, color, style, parent),
	_graph(graph),
	_secondaryGraph(NULL)
{
	Q_ASSERT(_graph.isValid());
	updateG();
}

bool GraphItem0::hasTime() const
{
	return _graph.hasTime();
}

void GraphItem0::updatePath()
{
	_path = QPainterPath();

	if (!(gType() == Time && !hasTime())) {
		for (int i = 0; i < _graph.size(); i++) {
			const GraphSegment &segment = _graph.at(i);

			_path.moveTo(segment.first().x(gType()),
					     segment.first().y());
			for (int i = 1; i < segment.size(); i++)
				_path.lineTo(segment.at(i).x(gType()),
						 segment.at(i).y());
		}
	}
}

void GraphItem0::updateBounds()
{
	if (gType() == Time && !hasTime()) {
		_bounds = QRectF();
		return;
	}

	qreal bottom, top, left, right;

	QPointF p = QPointF(_graph.first().first().x(gType()),
	                    _graph.first().first().y());
	bottom = top = p.y();
	left = right = p.x();

	for (int i = 0; i < _graph.size(); i++) {
		const GraphSegment &segment = _graph.at(i);

		for (int j = 0; j < segment.size(); j++) {
			p      = QPointF(segment.at(j).x(gType()), segment.at(j).y());
			top    = qMin(top,    p.y());
			bottom = qMax(bottom, p.y());
			left   = qMin(left,   p.x());
			right  = qMax(right,  p.x());
		}
	}

	if (left == right)
		_bounds = QRectF();
	else
		_bounds = QRectF(QPointF(left, top), QPointF(right, bottom));
}


const GraphSegment *GraphItem0::segment(qreal x, GraphType type) const
{
	for (int i = 0; i < _graph.size(); i++)
		if (x <= _graph.at(i).last().x(type))
			return &(_graph.at(i));

	return 0;
}

qreal GraphItem0::yAtX(qreal x) const
{
	const GraphSegment *seg = segment(x, gType());
	if (!seg)
		return NAN;

	int low = 0;
	int high = seg->count() - 1;
	int mid = 0;

	if (!(x >= seg->at(low).x(gType()) && x <= seg->at(high).x(gType())))
		return NAN;

	while (low <= high) {
		mid = low + ((high - low) / 2);
		const GraphPoint &p = seg->at(mid);
		if (p.x(gType()) > x)
			high = mid - 1;
		else if (p.x(gType()) < x)
			low = mid + 1;
		else
			return p.y();
	}

	QLineF l;
	if (seg->at(mid).x(gType()) < x)
		l = QLineF(seg->at(mid).x(gType()), seg->at(mid).y(),
		  seg->at(mid+1).x(gType()), seg->at(mid+1).y());
	else
		l = QLineF(seg->at(mid-1).x(gType()), seg->at(mid-1).y(),
		  seg->at(mid).x(gType()), seg->at(mid).y());

	return l.pointAt((x - l.p1().x()) / (l.p2().x() - l.p1().x())).y();
}

qreal GraphItem0::distanceAtTime(qreal time) const
{
	const GraphSegment *seg = segment(time, Time);
	if (!seg)
		return NAN;

	int low = 0;
	int high = seg->count() - 1;
	int mid = 0;

	if (!(time >= seg->at(low).t() && time <= seg->at(high).t()))
		return NAN;

	while (low <= high) {
		mid = low + ((high - low) / 2);
		const GraphPoint &p = seg->at(mid);
		if (p.t() > time)
			high = mid - 1;
		else if (p.t() < time)
			low = mid + 1;
		else
			return seg->at(mid).s();
	}

	QLineF l;
	if (seg->at(mid).t() < time)
		l = QLineF(seg->at(mid).t(), seg->at(mid).s(), seg->at(mid+1).t(),
		  seg->at(mid+1).s());
	else
		l = QLineF(seg->at(mid-1).t(), seg->at(mid-1).s(),
		  seg->at(mid).t(), seg->at(mid).s());

	return l.pointAt((time - l.p1().x()) / (l.p2().x() - l.p1().x())).y();
}

qreal GraphItem0::avg() const
{
	qreal sum = 0;

	for (int i = 0; i < _graph.size(); i++) {
		const GraphSegment &segment = _graph.at(i);

		for (int j = 1; j < segment.size(); j++)
			sum += segment.at(j).y() * (segment.at(j).s() - segment.at(j-1).s());
	}

	return sum/_graph.last().last().s();
}

