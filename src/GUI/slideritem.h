#ifndef SLIDERITEM_H
#define SLIDERITEM_H

#include <QGraphicsObject>
#include <QPen>

#include "common/range.h"

class QColor;

class SliderItem : public QGraphicsObject
{
	Q_OBJECT

public:
	SliderItem(QGraphicsItem *parent = 0);

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
	  QWidget *widget);

	const RangeF &range() const {return _range;}

	void setRange(const RangeF &range);
	void setSliderHeight(qreal height);
	void setSliderFixY(qreal y);

	void setColor(const QColor &color);

	void clear();

signals:
	void positionChanged(const QPointF&);

protected:
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);

private:
	RangeF _range;
	QPen   _pen;
	qreal  _fixY;
	qreal  _height;
};

#endif // SLIDERITEM_H
