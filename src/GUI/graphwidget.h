#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QGraphicsView>
#include <QList>
#include <QVector>

#include "graphview.h"
#include "graphicsscene.h"
#include "palette.h"
#include "units.h"
#include "common/range.h"

/*
 * Widgets for GraphView
 */

class GraphWidget;
class GraphContent;
class AxisWidget;


class GraphWidget : public QWidget
{
	Q_OBJECT

	friend GraphContent;
	friend GraphView;

public:
	GraphWidget(QWidget *parent = 0);
	~GraphWidget();
	
	class Ticks
	{
	public:
		Ticks() : _min(0.), _d(1.), _sub(-1) {}
		Ticks(RangeF values, int maxCount);

		int count() const {return _sub + 1;}
		int sub() const {return _sub;}
		double min() const {return _min;}
		double max() const {return _min + _sub * _d;}
		int after(double v) const {return (int)ceil((v - _min) / _d);}
		int before(double v) const {return (int)floor((v - _min) / _d);}

		double operator[](int i) const {return _min + i * _d;}

	private:
		double _min;
		double _d;
		int    _sub;
	};

	struct Tick
	{
		// in widget coord, relative to content origin
		int   pos;
		// in unit
		qreal uval;
	};

protected:
	void resizeEvent(QResizeEvent *e);

private slots:
	void viewPosChange();

private:
	void addGraph(GraphItem *graph);
	void removeGraph(GraphItem *graph);
	void setGraphType(GraphType type);
	void setXUnits();
	void setUnits(Units units);
	void createXLabel();
	void createYLabel();
	void setSliderPosition(qreal x);
	void updateSliderInfo();
	void updateLayout();

	QPointF pointToUnit(const QPointF&);
	QPointF pointFromUnit(const QPointF&);


	QList<GraphItem*> _graphs;

	GraphContent *_content;
	AxisWidget   *_xAxis, *_yAxis;
	
	// datas bounds
	QRectF _bounds;

	qreal _xScale;
	qreal _yScale, _yOffset;

	QString _xUnits, _yUnits;
	QString _xLabel, _yLabel;

	Ticks         _xTicks,  _yTicks; // in unit
	QVector<Tick> _vxTicks, _vyTicks;
	
	GraphType _graphType;
	Units     _units;
	int       _precision;
	qreal     _minYRange;
	bool      _showGrid;
	bool      _showZero;
};


/*
 * Graph items are in scene coordinates which are the data coordinates
 * y-axis is inverted when displaying so _bounds.top() (i.e. min(y))
 * is mapped to rect().bottom() in view coordinates
 */
class GraphContent : public QGraphicsView
{
	Q_OBJECT

	friend GraphWidget;
	friend GraphView;

public:
	virtual ~GraphContent();

private:
	GraphContent(GraphWidget* widgt);

	void removeItem(QGraphicsItem *item);
	void addItem(QGraphicsItem *item);

	QRectF visibleRect();

protected:
	void drawBackground(QPainter *painter, const QRectF& rect);

	void changeEvent(QEvent *e);
	void mousePressEvent(QMouseEvent *e);
	void wheelEvent(QWheelEvent *e);

private:

	GraphWidget    *_widgt;
	GraphicsScene  *_scene;
	
	SliderItem     *_slider;
	SliderInfoItem *_sliderInfo;
};


class AxisWidget : public QWidget
{
public:
	enum Type {X, Y};

	AxisWidget(Type type, QWidget* parent = 0);

	void  setTicks(const QVector<GraphWidget::Tick>* ticks);
	void  setLen(int len);
	void  setLabel(const QString& label);

	//centered on origin
	QRect boundingRect() const {return _boundingRect;}
	// for x axis, ticks independant
	int   xBottomSpace() const {return _boundingRect.bottom();}
	void  setGeometry(const QRect &r, const QPoint origin);

protected:
	void  paintEvent(QPaintEvent *e);

private:
	void  updateBoundingRect();

	Type    _type;
	QPoint  _origin;
	int     _len;
	QString _label;
	QRect   _labelBB;
	QRect   _boundingRect;
	QFont   _font;
	int     _font_maxw;
	QLocale _locale;
	
	const QVector<GraphWidget::Tick>* _ticks;
};

#endif
