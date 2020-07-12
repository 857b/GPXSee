#include <QPainter>
#include <QTransform>
#include <QWidget>
#include "slideritem.h"

#define SIZE        10

SliderItem::SliderItem(QGraphicsItem *parent) : QGraphicsObject(parent)
{
	setFlag(ItemIsMovable);
	setFlag(ItemSendsGeometryChanges);

	_color  = Qt::red;
	_height = 0;
	_fixY   = 0;
}

QRectF SliderItem::boundingRect() const
{
	return QRectF(-SIZE/2, 0, SIZE, _height);
}

void SliderItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
  QWidget *widget)
{
	Q_UNUSED(option);

	painter->setRenderHint(QPainter::Antialiasing, false);
	painter->setPen(_color);
	painter->drawLine(QLineF(0, 0, 0, _height));

	if (widget) {
		QTransform tr = painter->worldTransform();
		painter->resetTransform();
		int x = tr.map(QPointF(0, 0)).x();
		painter->drawLine(x-SIZE/2, 0, x+SIZE/2, 0);
		painter->drawLine(x-SIZE/2, widget->height() - 1,
						  x+SIZE/2, widget->height() - 1);
		painter->setWorldTransform(tr);
	}

//	painter->drawRect(boundingRect());
}

QVariant SliderItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemPositionChange && scene()) {
		QPointF pos = value.toPointF();
		pos.setY(_fixY);
		pos.setX(_range.into(pos.x()));
		return pos;
	}

	if (change == ItemPositionHasChanged && scene())
		emit positionChanged(value.toPointF());

	return QGraphicsItem::itemChange(change, value);
}

void SliderItem::clear()
{
	_range = RangeF();
	setPos(QPointF());
}

void SliderItem::setRange(const RangeF &range)
{
	prepareGeometryChange();
	_range = range;
}

void SliderItem::setSliderHeight(qreal height) {
	prepareGeometryChange();
	_height = height;
}

void SliderItem::setSliderFixY(qreal y) {
	_fixY = y;
	setY(y);
}

void SliderItem::setColor(const QColor &color)
{
	_color = color;
	update();
}
