#include <QPainter>
#include "font.h"
#include "sliderinfoitem.h"


#define SIZE 5

SliderInfoItem::SliderInfoItem(QGraphicsItem *parent) : QGraphicsItem(parent)
{
	setFlag(ItemIgnoresTransformations);

	_side = Right;
	_color = Qt::red;

	_font.setPixelSize(FONT_SIZE);
	_font.setFamily(FONT_FAMILY);
}

void SliderInfoItem::updateBoundingRect()
{
	QFontMetrics fm(_font);

	qreal width = qMax(fm.width(_x), fm.width(_y));
	qreal height = 2 * fm.height() - 2*fm.descent();

	_boundingRect = _side == Right
	  ? QRectF(-SIZE/2, _top, width + 1.5*SIZE, height)
	  : QRectF(-(width + SIZE), _top, width + 1.5*SIZE, height);
}

void SliderInfoItem::paint(QPainter *painter, const QStyleOptionGraphicsItem
  *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	QFontMetrics fm(_font);
	QRectF rx, ry;


	int width = qMax(fm.width(_x), fm.width(_y)),
		fh    = fm.height(),
		fd    = fm.descent(),
		semih = fh - fd;
	
	if (_side == Right) {
		rx = QRect(SIZE,  0,     fm.width(_x), semih);
		ry = QRectF(SIZE, semih, fm.width(_y), semih);
	} else {
		rx = QRectF(-(width + SIZE), 0,     fm.width(_x), semih);
		ry = QRectF(-(width + SIZE), semih, fm.width(_y), semih);
	}

	rx.translate(0, _top);
	ry.translate(0, _top);

	painter->setPen(Qt::NoPen);
	QColor bc(painter->background().color());
	bc.setAlpha(196);
	painter->setBrush(QBrush(bc));
	painter->drawRect(rx);
	painter->drawRect(ry);
	painter->setBrush(Qt::NoBrush);

	painter->setFont(_font);
	painter->setRenderHint(QPainter::Antialiasing, false);
	painter->setPen(_color);

	if (_side == Right) {
		painter->drawText(SIZE, _top +     fh - 0.5 * fd, _x);
		painter->drawText(SIZE, _top + 2 * fh - 2.5 * fd, _y);
	} else {
		painter->drawText(-(width + SIZE), _top +     fh - 0.5 * fd, _x);
		painter->drawText(-(width + SIZE), _top + 2 * fh - 2.5 * fd, _y);
	}
	painter->drawLine(QPointF(-SIZE/2, 0), QPointF(SIZE/2, 0));

//	painter->drawRect(boundingRect());
}

void SliderInfoItem::setText(const QString &x, const QString &y)
{
	prepareGeometryChange();
	_x = x; _y = y;
	updateBoundingRect();
	update();
}

void SliderInfoItem::setSide(Side side)
{
	if (side == _side)
		return;

	prepareGeometryChange();
	_side = side;
	updateBoundingRect();
	update();
}

void SliderInfoItem::setColor(const QColor &color)
{
	_color = color;
	update();
}

void SliderInfoItem::setTop(int top) {
	prepareGeometryChange();
	_top = top;
	updateBoundingRect();
	update();
}
