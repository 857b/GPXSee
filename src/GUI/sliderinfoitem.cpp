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

	qreal width = qMax(fm.width(_d), qMax(fm.width(_x), fm.width(_y)));
	int lc = _d.isEmpty() ? 2 : 3;
	qreal height = lc * (fm.height() - fm.descent());

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
	QRectF rx, ry, rd;

	bool hasD = !_d.isEmpty();

	int xW    = fm.width(_x),
		yW    = fm.width(_y),
		dW    = hasD ? fm.width(_d) : 0,
		width = qMax(dW, qMax(xW, yW)),
		fh    = fm.height(),
		fd    = fm.descent(),
		semih = fh - fd;

	if (_side == Right) {
		rx = QRect(SIZE,  0,       xW, semih);
		ry = QRectF(SIZE, semih,   yW, semih);
		rd = QRectF(SIZE, 2*semih, dW, semih);
	} else {
		rx = QRectF(-(width + SIZE), 0,       xW, semih);
		ry = QRectF(-(width + SIZE), semih,   yW, semih);
		rd = QRectF(-(width + SIZE), 2*semih, yW, semih);
	}

	rx.translate(0, _top);
	ry.translate(0, _top);
	rd.translate(0, _top);

	painter->setPen(Qt::NoPen);
	QColor bc(painter->background().color());
	bc.setAlpha(196);
	painter->setBrush(QBrush(bc));
	painter->drawRect(rx);
	painter->drawRect(ry);
	if (hasD)
		painter->drawRect(rd);
	painter->setBrush(Qt::NoBrush);

	painter->setFont(_font);
	painter->setRenderHint(QPainter::Antialiasing, false);
	painter->setPen(_color);

	if (_side == Right) {
		painter->drawText(SIZE, _top +     fh - 0.5 * fd, _x);
		painter->drawText(SIZE, _top + 2 * fh - 2.5 * fd, _y);
		if (hasD)
			painter->drawText(SIZE, _top + 3 * fh - 4.5 * fd, _d);
	} else {
		painter->drawText(-(width + SIZE), _top +     fh - 0.5 * fd, _x);
		painter->drawText(-(width + SIZE), _top + 2 * fh - 2.5 * fd, _y);
		if (hasD)
			painter->drawText(-(width + SIZE), _top + 3 * fh - 4.5 * fd, _d);
	}
	painter->drawLine(QPointF(-SIZE/2, 0), QPointF(SIZE/2, 0));

//	painter->drawRect(boundingRect());
}

void SliderInfoItem::setText(const QString &x, const QString &y,
							 const QString &d)
{
	prepareGeometryChange();
	_x = x; _y = y; _d = d;
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
