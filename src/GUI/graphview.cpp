#include "graphview.h"

#include <QVBoxLayout>
#include <QSizePolicy>
#include <QSet>
#include <QStyleOptionGraphicsItem>
#include <QEvent>
#include <QMouseEvent>
#include <QGraphicsSimpleTextItem>
#include <QPalette>
#include <QLocale>
#include <QFont>

#include "data/graph.h"
#include "opengl.h"
#include "slideritem.h"
#include "sliderinfoitem.h"
#include "infoitem.h"
#include "graphitem.h"
#include "graphicsscene.h"
#include "graphwidget.h"

GraphView::GraphView(QWidget *parent)
	: QWidget(parent),
	  _widgt(new GraphWidget(this)),
	  _info(new QLabel())
{
	QVBoxLayout* vl = new QVBoxLayout;
	vl->setContentsMargins(0,5,0,0);
	vl->setSpacing(2);
	QFont info_font;
	info_font.setPointSize(10);
	_info->setTextInteractionFlags(Qt::TextSelectableByMouse);
	_info->setFont(info_font);
	_info->setAlignment(Qt::AlignCenter);
	_info->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	vl->addWidget(_info);
	vl->addWidget(_widgt);
	setLayout(vl);

	_sliderPos = 0;
	_width     = 1;

	connect(_widgt->_content->_slider,
				SIGNAL(positionChanged(const QPointF&)),
			this, SLOT(emitSliderPositionChanged(const QPointF&)));
}

bool GraphView::isEmpty() const
{
	const QList<GraphItem*>& gs(_widgt->_graphs);
	if (graphType() == Distance)
		return gs.isEmpty();
	for (int i = 0; i < gs.size(); ++i)
		if (gs.at(i)->hasTime())
			return false;
	return true;
}

void GraphView::clear()
{
	_widgt->_graphs.clear();

	_widgt->_content->_slider->clear();
	_infos.clear();
	_info->clear();

	_palette.reset();

	_widgt->_bounds = QRectF();
	_sliderPos = 0;

	_widgt->_content->_scene->setSceneRect(0, 0, 0, 0);
}

void GraphView::plot(QPainter *painter, const QRectF &target, qreal scale)
{
	//TODO
	Q_UNUSED(scale);
	_widgt->setUpdatesEnabled(false);
	SliderItem* s = _widgt->_content->_slider;

	if (s->pos().x() == s->range().min()) s->hide();
	_widgt->_content->_scene->render(painter, target);
	s->show();

	_widgt->setUpdatesEnabled(true);
}


const QString& GraphView::yLabel() const {return _widgt->_yLabel;}
const QString& GraphView::yUnits() const {return _widgt->_yUnit.name;}
qreal GraphView::yScale() const {return _widgt->_yUnit.offset;}
qreal GraphView::yOffset() const {return _widgt->_yUnit.offset;}
const Unit& GraphView::yUnit() const {return _widgt->_yUnit;}

void GraphView::setYLabel(const QString &label)
{
	_widgt->_yLabel = label;
	_widgt->createYLabel();
}

void GraphView::setYUnit(const Unit& unit)
{
	_widgt->_yUnit = unit;
	_widgt->createYLabel();//updateLayout
}

void GraphView::setSliderFmt(const Unit::Fmt& fmt)
{
	_widgt->_slidderFmt = fmt;
	_widgt->updateSliderInfo();
}

void GraphView::setMinYRange(qreal range)
{
	_widgt->_minYRange = range;
}

Units GraphView::units() const
{
	return _widgt->_units;
}

void GraphView::setUnits(Units units)
{
	_widgt->setUnits(units);
}

GraphType GraphView::graphType() const
{
	return _widgt->_graphType;
}

void GraphView::setGraphType(GraphType type)
{
	_widgt->setGraphType(type);
}

void GraphView::showGrid(bool show)
{
	_widgt->_showGrid = show;
	_widgt->_content->resetCachedContent();
	_widgt->_content->update();
	//BUG: drawBackground is not called
	// -> updateLayout to force update
	_widgt->updateLayout();
}

void GraphView::showZero(bool show)
{
	_widgt->_showZero = show;
	_widgt->_content->resetCachedContent();
	_widgt->_content->update();
	_widgt->updateLayout();
}

void GraphView::showSliderInfo(bool show)
{
	_widgt->_content->_sliderInfo->setVisible(show);
}

void GraphView::addGraph(GraphItem *graph)
{
	connect(this, SIGNAL(sliderPositionChanged(qreal)), graph,
	  SLOT(emitSliderPositionChanged(qreal)));
	graph->setWidth(_width);
	_widgt->addGraph(graph);
}

void GraphView::removeGraph(GraphItem *graph)
{
	disconnect(this, SIGNAL(sliderPositionChanged(qreal)), graph,
	  SLOT(emitSliderPositionChanged(qreal)));

	_widgt->removeGraph(graph);
}

std::pair<GraphItem*, GraphItem*> GraphView::mainGraphs()
{
	QList<GraphItem*>& gs(_widgt->_graphs);
	GraphItem *g1, *g2;
	if (gs.count() == 1)
		return std::pair<GraphItem*, GraphItem*>(gs.first(), NULL);
	else if (gs.count() == 2 && (g2 = (g1 = gs.first())->secondaryGraph()))
		return std::pair<GraphItem*, GraphItem*>(g1, g2);
	else
		return std::pair<GraphItem*, GraphItem*>(NULL, NULL);
}

QRectF GraphView::bounds() const
{
	return _widgt->_bounds;
}

void GraphView::redraw()
{
	_widgt->updateLayout();
}


void GraphView::emitSliderPositionChanged(const QPointF &pos)
{
	if (_widgt->_content->_slider->range().size() <= 0)
		return;

	_sliderPos = pos.x();
	_sliderPos = qMax(_sliderPos, bounds().left());
	_sliderPos = qMin(_sliderPos, bounds().right());
	_widgt->updateSliderInfo();

	emit sliderPositionChanged(_sliderPos);
}

void GraphView::setSliderPosition(qreal pos)
{
	_sliderPos = pos;
	_widgt->setSliderPosition(pos);
}

void GraphView::newSliderPosition(const QPointF &pos)
{
	setSliderPosition(pos.x());
}

void GraphView::addInfo(const QString &key, const QString &value)
{
	KV<QString, QString> kv(key, value);
	int i = _infos.indexOf(kv);
	if (i < 0)
		_infos.append(kv);
	else
		_infos[i] = kv;
	updateInfo();
}

void GraphView::clearInfo()
{
	_infos.clear();
	_info->clear();
}

void GraphView::updateInfo()
{
	QString text;
	for (QList<KV<QString, QString> >::const_iterator i = _infos.constBegin();
		  i != _infos.constEnd(); i++) {
		text.append(i->key());
		text.append(": ");
		text.append(i->value());
		if (i != _infos.constEnd() - 1)
			text.append(" | ");
	}
	_info->setText(text);
	_info->setTextFormat(Qt::PlainText);
}

void GraphView::onMainGraphChanged()
{
	_widgt->updateSliderInfo();
}

void GraphView::setPalette(const Palette &palette)
{
	_palette = palette;
	_palette.reset();

	QSet<GraphItem*> secondary;
	for (int i = 0; i < _widgt->_graphs.count(); i++) {
		GraphItem *g = _widgt->_graphs[i];
		if (g->secondaryGraph())
			secondary.insert(g->secondaryGraph());
	}

	for (int i = 0; i < _widgt->_graphs.count(); i++) {
		GraphItem *g = _widgt->_graphs[i];
		if (secondary.contains(g))
			continue;

		QColor color(_palette.nextColor());
		g->setColor(color);
		if (g->secondaryGraph())
			g->secondaryGraph()->setColor(color);
	}
}

void GraphView::setGraphWidth(int width)
{
	_width = width;

	for (int i = 0; i < _widgt->_graphs.count(); i++)
		_widgt->_graphs.at(i)->setWidth(width);
}

void GraphView::setRenderHint(QPainter::RenderHint hint, bool enabled) {
	_widgt->_content->setRenderHint(hint, enabled);
}

void GraphView::useOpenGL(bool use)
{
	if (use)
		_widgt->_content->setViewport(new OPENGL_WIDGET);
	else
		_widgt->_content->setViewport(new QWidget);
}

void GraphView::useAntiAliasing(bool use)
{
	_widgt->_content->setRenderHint(QPainter::Antialiasing, use);
}

void GraphView::setSliderColor(const QColor &color)
{
	_widgt->_content->_slider->setColor(color);
	_widgt->_content->_sliderInfo->setColor(color);
}
