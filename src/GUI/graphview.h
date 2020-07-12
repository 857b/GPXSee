#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <QWidget>
#include <QPainter>
#include <QLabel>

#include "data/graph.h"
#include "common/range.h"
#include "common/kv.h"
#include "palette.h"
#include "units.h"
#include "infoitem.h"

class GraphContent;
class SliderItem;
class SliderInfoItem;
class GraphItem;
class PathItem;
class QGraphicsSimpleTextItem;
class GraphWidget;


class GraphView : public QWidget
{
	Q_OBJECT

	friend GraphContent;

public:
	GraphView(QWidget *parent = 0);
	~GraphView();

	bool isEmpty() const;
	const QList<KV<QString, QString> > &info() const {return _infos;}
	void clear();

	void plot(QPainter *painter, const QRectF &target, qreal scale);

	void setPalette(const Palette &palette);
	void setGraphWidth(int width);
	void setRenderHint(QPainter::RenderHint hint, bool enabled=true);
	void showGrid(bool show = true);
	void showZero(bool show = true);
	void showSliderInfo(bool show = true);
	void useOpenGL(bool use = true);
	void useAntiAliasing(bool use = true);

	void setSliderPosition(qreal pos);
	void setSliderColor(const QColor &color);

signals:
	void sliderPositionChanged(qreal);

protected:
	void addGraph(GraphItem *graph);
	void removeGraph(GraphItem *graph);
	GraphType graphType() const;
	void setGraphType(GraphType type);
	Units units() const;
	void setUnits(Units units);

	void changeEvent(QEvent *e);

	const QString &yLabel() const;
	const QString &yUnits() const;
	qreal yScale() const;
	qreal yOffset() const;
	void setYLabel(const QString &label);
	void setYUnits(const QString &units);
	void setYScale(qreal scale);
	void setYOffset(qreal offset);

	void setSliderPrecision(int precision);
	void setMinYRange(qreal range);

	QRectF bounds() const;
	void redraw();
	void addInfo(const QString &key, const QString &value);
	void clearInfo();
	void updateInfo();

protected:
	Palette _palette;
	int     _width;

private slots:
	void emitSliderPositionChanged(const QPointF &pos);
	void newSliderPosition(const QPointF &pos);

private:
	GraphWidget *_widgt;
	QLabel      *_info;

	QList<KV<QString, QString> > _infos;
	QGraphicsSimpleTextItem     *_message;//TODO
	
	qreal   _sliderPos;
};

#endif // GRAPHVIEW_H
