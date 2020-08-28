#ifndef GRAPHITEM_H
#define GRAPHITEM_H

#include <QGraphicsObject>
#include <QPen>
#include <QDateTime>
#include "data/graph.h"
#include "units.h"
#include "graphicsscene.h"

class GraphItem : public QObject, public GraphicsItem
{
	Q_OBJECT

public:
	GraphItem(QObject* oParent, GraphType type,
			int width, const QColor &color, Qt::PenStyle style,
			QGraphicsItem *iParent = 0);
	virtual ~GraphItem() {}

	virtual QString info() const = 0;

	QPainterPath shape() const {return _shape;}
	QRectF boundingRect() const {return _shape.boundingRect();}
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
	  QWidget *widget);

	const QRectF &bounds() const {return _bounds;}
	virtual bool hasTime() const = 0;
	GraphType gType() const {return _type;}

	void setGraphType(GraphType type);
	void setColor(const QColor &color);
	void setWidth(int width);
	void setShapeWidth(qreal shpWidth);
	void setUnits(Units units) {_units = units;}

	virtual GraphItem *secondaryGraph() const = 0;

	virtual qreal     yAtX(qreal x) const = 0;
	virtual qreal     distanceAtTime(qreal time) const = 0;
	virtual QDateTime dateAtX(qreal x) const
		{Q_UNUSED(x); return QDateTime();}
	
	qreal min() const {return _bounds.top();}
	qreal max() const {return _bounds.bottom();}

	void redraw();

signals:
	void sliderPositionChanged(qreal);
	void selected(bool);

public slots:
	void emitSliderPositionChanged(qreal);
	void hover(bool hover);

protected:
	void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);

	void updateG();
	virtual void updatePath()   = 0;
	virtual void updateBounds() = 0;

	Units        _units;
	QPainterPath _path;
	QRectF       _bounds;

private:
	void updateShape();

	GraphType    _type;
	QPainterPath _shape;
	QPen         _pen;
	qreal        _shpWidth;
};

class GraphItem0 : public GraphItem
{
public:
	GraphItem0(const Graph &graph, GraphType type, int width,
	  const QColor &color, Qt::PenStyle style, QGraphicsItem *parent = 0);

	virtual bool hasTime() const;
	
	virtual GraphItem *secondaryGraph() const {return _secondaryGraph;}
	void setSecondaryGraph(GraphItem *graph) {_secondaryGraph = graph;}

	virtual qreal yAtX(qreal x) const;
	virtual qreal distanceAtTime(qreal time) const;

	qreal avg() const;

protected:
	virtual void updatePath();
	virtual void updateBounds();

private:
	const GraphSegment *segment(qreal x, GraphType type) const;


	Graph      _graph;
	GraphItem *_secondaryGraph;
};

#endif // GRAPHITEM_H
