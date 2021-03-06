#include "graphwidget.h"

#include <QEvent>
#include <QMouseEvent>
#include <QScrollBar>

#include "common/util.h"
#include "font.h"
#include "format.h"
#include "slideritem.h"
#include "sliderinfoitem.h"
#include "graphitem.h"

#define AXIS_WIDTH		1
// len of the ticks marks
#define AXIS_TICK   	6
#define AXIS_PADDING	6
#define MAX_TICK_LEN	5

// limit the number of y-ticks
#define YTICKS_MAX_COUNT 20
#define YTICKS_MIN_SPACE 15.

#define X_PADDING     5
#define Y_PADDING     5
#define VIEW_MARGIN .05

GraphWidget::Ticks::Ticks(RangeF values, int maxCount)
{
	double range = niceNum(values.max() - values.min(), false);
	_d = niceNum(range / maxCount, false);
	_min = ceil(values.min() / _d) * _d;
	_sub = (int)floor((values.max() - _min) / _d);
}

GraphWidget::GraphWidget(GraphView *gview, QWidget *parent)
	: QWidget(parent),
	  _gview(gview),
	  _content(new GraphContent(this)),
	  _xAxis(new AxisWidget(AxisWidget::X, this)),
	  _yAxis(new AxisWidget(AxisWidget::Y, this))
{
	connect(_content->horizontalScrollBar(), SIGNAL(valueChanged(int)),
			this, SLOT(viewPosChange()));

	_slidderFmt.prec = 0;
	_minYRange = 0.01;

	_showGrid  = true;
	_showZero  = false;

	_units     = Metric;
	_graphType = Distance;
	_xLabel    = tr("Distance");
}

GraphWidget::~GraphWidget() {}

void GraphWidget::addGraph(GraphItem *graph)
{
	_graphs.append(graph);
	if (!graph->bounds().isNull())
		_content->addItem(graph);
	_bounds |= graph->bounds();

	setXUnits();
	updateLayout();
}

void GraphWidget::removeGraph(GraphItem *graph)
{
	_graphs.removeOne(graph);
	_content->removeItem(graph);

	_bounds = QRectF();
	for (int i = 0; i < _graphs.count(); i++)
		_bounds |= _graphs.at(i)->bounds();

	setXUnits();

	updateLayout();
}

void GraphWidget::setGraphType(GraphType type)
{
	_graphType = type;
	_bounds = QRectF();

	for (int i = 0; i < _graphs.count(); i++) {
		GraphItem *gi = _graphs.at(i);
		gi->setGraphType(type);
		if (gi->bounds().isNull())
			_content->removeItem(gi);
		else
			_content->addItem(gi);
		_bounds |= gi->bounds();
	}

	if (type == Distance)
		_xLabel = tr("Distance");
	else
		_xLabel = tr("Time");
	setXUnits();
}

void GraphWidget::setUnits(Units units)
{
	_units = units;

	for (int i = 0; i < _graphs.count(); i++)
		_graphs.at(i)->setUnits(units);

	setXUnits();

	updateLayout();
}

void GraphWidget::setXUnits()
{
	qreal span = _bounds.width();
	_xUnit = _graphType == Distance
		? Unit::distance(_units, span)
		: Unit::time(span);
	createXLabel();
}

void GraphWidget::createXLabel()
{
	_xAxis->setLabel(QString("%1 [%2]")
		.arg(_xLabel, _xUnit.name.isEmpty() ? "-" : _xUnit.name));
	updateLayout();
}

void GraphWidget::createYLabel()
{
	_yAxis->setLabel(QString("%1 [%2]")
		.arg(_yLabel, _yUnit.name.isEmpty() ? "-" : _yUnit.name));
	updateLayout();
}

static qreal lim_margin(qreal x, qreal m)
{
	return x*(x + m) > 0 ? x + m : 0;
}

static QVector<GraphWidget::Tick> vTicks(GraphWidget::Ticks ts,
											RangeF ur, Range pr) {
	QVector<GraphWidget::Tick> rt;
	struct GraphWidget::Tick t;
	for (int i = ts.after(ur.min());
			i < ts.count() && (t.uval = ts[i]) <= ur.max(); ++i) {
		t.pos = pr.min() + (int)(pr.size() * (t.uval - ur.min()) / ur.size());
		rt.append(t);
	}
	return rt;
}

void GraphWidget::updateLayout()
{
	if (_bounds.isEmpty()) {
		hide();
		return;
	}
	
	QRectF br = _bounds;
	
	qreal margin = br.height() * VIEW_MARGIN;
	br.setTop(lim_margin(br.top(), -margin));
	br.setBottom(lim_margin(br.bottom(), margin));

	if (br.height() < _minYRange)
		br.adjust(0, -(_minYRange/2 - br.height()/2), 0,
				  _minYRange/2 - br.height()/2);
	
	QRect crect = rect();

	int my = _xAxis->xBottomSpace();
	crect.adjust(0, Y_PADDING, 0, -my-Y_PADDING);

	RangeF uyr = _yUnit.toUnit(RangeF(br.top(), br.bottom()));
	_yTicks  = Ticks(uyr, qMin(YTICKS_MAX_COUNT,
				            (int)floor(crect.height() / YTICKS_MIN_SPACE)));
	_zeroPos = uyr.max() * crect.height() / uyr.size();
	_vyTicks = vTicks(_yTicks, uyr, Range(0, crect.height()));
	_yAxis->setLen(crect.height());
	_yAxis->setTicks(&_vyTicks);
	
	int mx = (int)ceil(-_yAxis->boundingRect().left());
	crect.adjust(mx + X_PADDING, 0, -X_PADDING, 0);
	_content->setGeometry(crect);

	QRectF vr = _content->visibleRect();

	vr.setTop(br.top());
	vr.setBottom(br.bottom());
	if (vr.left() < br.left())
		vr.moveLeft(br.left());
	if (vr.right() > br.right())
		vr.moveRight(br.right());
	if (vr.left() < br.left())
		vr.setLeft(br.left());
	if (vr.width() == 0)
		vr = br;

	qreal sx = crect.width()  / vr.width(),
	      sy = crect.height() / vr.height();

	_content->setTransform(QTransform(
			sx, 0., 0.,
			0., -sy, 0.,
			- vr.left() * sx, vr.top()  * sy, 1.),
		false);

	RangeF uxr = _xUnit.toUnit(RangeF(br.left(), br.right()));
	_xTicks = Ticks(uxr, (int)floor(br.width() * sx / 50.));

	RangeF uvxr = _xUnit.toUnit(RangeF(vr.left(), vr.right()));
	_vxTicks = vTicks(_xTicks, uvxr, Range(0, crect.width()));
	_xAxis->setLen(crect.width());
	_xAxis->setTicks(&_vxTicks);

	QRect axr = _xAxis->boundingRect();
	_xAxis->setGeometry(axr.translated(crect.bottomLeft()),
						-axr.topLeft());
	axr = _yAxis->boundingRect();
	_yAxis->setGeometry(axr.translated(crect.bottomLeft()),
						-axr.topLeft());
	
	_content->_slider->setRange(RangeF(br.left(), br.right()));
	_content->_slider->setSliderHeight(qMax(0., br.height()));
	_content->_slider->setSliderFixY(br.top());

	_content->setSceneRect(br);
	setSliderPosition(_content->_slider->x());

	for (int i = 0; i < _graphs.size(); ++i)
		_graphs[i]->setShapeScale(sx, sy);

	update();
	show();
}

void GraphWidget::resizeEvent(QResizeEvent *e)
{
	updateLayout();

	QWidget::resizeEvent(e);
}

void GraphWidget::viewPosChange() {
	QRectF   vr = _content->visibleRect();
	RangeF uvxr = _xUnit.toUnit(RangeF(vr.left(), vr.right()));
	_vxTicks    = vTicks(_xTicks, uvxr, Range(0, _content->width()));
	_xAxis->setTicks(&_vxTicks);

	QRect   axr = _xAxis->boundingRect();
	_xAxis->setGeometry(axr.translated(_content->geometry().bottomLeft()),
						-axr.topLeft());
}

void GraphWidget::setSliderPosition(qreal x)
{
	if (x <= _bounds.right() && x >= _bounds.left()) {
		_content->_slider->setPos(x, _content->sceneRect().top());
		_content->_slider->setVisible(true);
		updateSliderInfo();
	} else {
		_content->_slider->setPos(_content->_slider->range().min(),
		                          _content->sceneRect().top());
		_content->_slider->setVisible(false);
	}
} 

void GraphWidget::updateSliderInfo()
{
	SliderInfoItem* s_i = _content->_sliderInfo;

	QLocale l(QLocale::system());
	qreal x = _content->_slider->x(),
		  y = _bounds.center().y();

	std::pair<GraphItem*, GraphItem*> mainG(_gview->mainGraphs());
	if (mainG.first) y = mainG.first->yAtX(x);

	QString xText(_graphType == Time
					? Format::timeSpan(x, _bounds.width() > 3600)
					: _xUnit.format(x, 1));
	QString yText(mainG.first
					? _yUnit.format(y, _slidderFmt)
					: QString());
	if (mainG.second) {
		qreal delta = y - mainG.second->yAtX(x);
		if (!std::isnan(delta)) {
			Unit::Fmt dFmt(_slidderFmt);
			dFmt.force_sign = true;
			yText += " " + QChar(0x0394)//Delta
				  + _yUnit.format(delta, dFmt);
		}
	}
	QString dText;
	if (mainG.first) {
		QDateTime dt = mainG.first->dateAtX(x);
		if (!dt.time().isNull())
			dText = dt.time().toString(Qt::TextDate);
	}
	s_i->setText(xText, yText, dText);

	QPoint wpos = _content->mapFromScene(QPointF(x, y));
	int right = wpos.x()
	          + (int)ceil(_content->_sliderInfo->boundingRect().width());
	s_i->setSide(right > _content->width()
	                   ? SliderInfoItem::Left : SliderInfoItem::Right);
	s_i->setPos(QPointF(0, y - _content->_slider->y()));

	int height = s_i->boundingRect().height();
	int top    = -height / 2;
	if (wpos.y() + top + height >= _content->height() - 1)
		top = _content->height() - height - wpos.y() - 2;
	if (wpos.y() + top <= 0)
		top = 1-wpos.y();
	s_i->setTop(top);
}

QPointF GraphWidget::pointToUnit(const QPointF& p)
{
	return QPointF(_xUnit.toUnit(p.x()), _yUnit.toUnit(p.y()));
}

QPointF GraphWidget::pointFromUnit(const QPointF& p)
{
	return QPointF(_xUnit.fromUnit(p.x()), _yUnit.fromUnit(p.y()));
}


// GraphContent

GraphContent::GraphContent(GraphWidget* widgt)
	: QGraphicsView(widgt),
	  _widgt(widgt),
	  _scene(new GraphicsScene(this)),
	  _slider(new SliderItem()),
	  _sliderInfo(new SliderInfoItem(_slider))
{
	setScene(_scene);

	setFrameShape(QFrame::NoFrame);
	setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
	setRenderHint(QPainter::Antialiasing, true);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setBackgroundBrush(QBrush(palette().brush(QPalette::Base)));
	setTransformationAnchor(AnchorUnderMouse);

	_slider->setZValue(4.0);
	_scene->addItem(_slider);
	_sliderInfo->setZValue(4.0);
}

GraphContent::~GraphContent()
{
	delete _slider;
	delete _scene;
}

void GraphContent::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::PaletteChange)
		setBackgroundBrush(QBrush(
					palette().brush(QPalette::Base)));

	QGraphicsView::changeEvent(e);
}

void GraphContent::removeItem(QGraphicsItem *item)
{
	if (item->scene() == _scene)
		_scene->removeItem(item);
}

void GraphContent::addItem(QGraphicsItem *item)
{
	if (item->scene() != _scene) {
		_scene->addItem(item);
		fitInView(sceneRect());
	}
}

QRectF GraphContent::visibleRect()
{
	return mapToScene(rect()).boundingRect();
}

void GraphContent::mousePressEvent(QMouseEvent *e)
{
	QGraphicsView::mousePressEvent(e);
	if (!e->isAccepted() && e->button() == Qt::LeftButton) {
		e->accept();
		_slider->setPos(mapToScene(e->pos()));
	}
}

void GraphContent::wheelEvent(QWheelEvent *e)
{
	e->accept();
	static int deg = 0;

	deg += e->delta() / 8;
	if (qAbs(deg) < 15) return;
	qreal zoom = deg > 0 ? 1.25 : 1. / 1.25;
	deg = 0;

	scale(zoom, 1.);
	_widgt->updateLayout();
}

void GraphContent::drawBackground(QPainter *painter, const QRectF& r)
{
	QGraphicsView::drawBackground(painter, r);

	// in content widget coords
	painter->resetTransform();

	if (_widgt->_showZero) {
		painter->setRenderHint(QPainter::Antialiasing, false);
		painter->setPen(Qt::gray);
		int v = _widgt->_zeroPos;
		painter->drawLine(rect().left(), v, rect().right(), v);
	}

	if (_widgt->_showGrid) {
		QBrush brush(Qt::gray);
		QPen pen = QPen(brush, 0, Qt::DotLine);

		painter->setRenderHint(QPainter::Antialiasing, false);
		painter->setPen(pen);

		for (int i = 0; i < _widgt->_vxTicks.size(); ++i) {
			int v = _widgt->_vxTicks[i].pos;
			painter->drawLine(v, rect().top(), v, rect().bottom());
		}
		for (int i = 0; i < _widgt->_vyTicks.size(); ++i) {
			int v = height() - 1 - _widgt->_vyTicks[i].pos;
			painter->drawLine(rect().left(), v, rect().right(), v);
		}
	}
	painter->setPen(Qt::gray);
	painter->drawRect(rect().adjusted(0, 0, -1, -1));
}

// Axis

AxisWidget::AxisWidget(Type type, QWidget *parent)
  : QWidget(parent), 
	_type(type),
	_len(0),
	_locale(QLocale::system()),
	_ticks(0)
{
	_font.setPixelSize(FONT_SIZE);
	_font.setFamily(FONT_FAMILY);
	
	_font_maxw = 0;
	QFontMetrics fm(_font);
	for (char c = 'a'; c <= 'z'; ++c)
		_font_maxw = qMax(_font_maxw, fm.width(QChar(c)));
	for (char c = '0'; c <= '9'; ++c)
		_font_maxw = qMax(_font_maxw, fm.width(QChar(c)));
	char oth[] = " .-+";
	for (char* c = oth; *c; ++c)
		_font_maxw = qMax(_font_maxw, fm.width(QChar(*c)));
}

void AxisWidget::setTicks(const QVector<GraphWidget::Tick>* ticks) {
	_ticks = ticks;
	updateBoundingRect();
	update();
}

void AxisWidget::setLen(int len)
{
	_len = len;
	updateBoundingRect();
	update();
}

void AxisWidget::setLabel(const QString& label)
{
	QFontMetrics fm(_font);
	_label = label;
	_labelBB = fm.tightBoundingRect(label);
	updateBoundingRect();
	update();
}

void AxisWidget::setGeometry(const QRect &r, const QPoint origin) {
	QWidget::setGeometry(r);
	_origin = origin;
	update();
}

void AxisWidget::updateBoundingRect()
{
	QFontMetrics fm(_font);
	int   th = fm.height();
	QRect ls(_labelBB);

	if (_type == X) {
		int minx = 0, maxx = _len;
		if (_ticks) for (int i = 0; i < _ticks->size(); ++i) {
			const GraphWidget::Tick& t(_ticks->at(i));
			int w = fm.width(_locale.toString(t.uval));
			minx = qMin(minx, t.pos - w / 2);
			maxx = qMax(maxx, t.pos + w / 2);
		}

		_boundingRect = QRect(
			qMin(minx, (_len - ls.width())/2),
			-AXIS_TICK/2,
			qMax(maxx, ls.width()),
			ls.height() + th - fm.descent()
				+ AXIS_TICK + 2*AXIS_PADDING + 1);
	} else {
		int mxw = 0;
		if (_ticks) for (int i = 0; i < _ticks->size(); ++i)
			mxw = qMax(mxw, fm.width(_locale.toString(_ticks->at(i).uval)));
		_boundingRect = QRect(
			-(ls.height() + mxw + 2*AXIS_PADDING + AXIS_TICK/2),
		    qMin((_len - ls.width())/2, -(_len + th/2 + fm.descent())),
			ls.height() + mxw + 2*AXIS_PADDING + AXIS_TICK,
			qMax(_len + th + fm.descent(), ls.width()));
	}
}

void AxisWidget::paintEvent(QPaintEvent *e) {
	Q_UNUSED(e);

	QPainter painter(this);
	painter.translate(_origin);
	QFontMetrics fm(_font);
	int th = fm.height();

	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setFont(_font);
	QPen pen(painter.pen());
	pen.setWidth(AXIS_WIDTH);
	pen.setCosmetic(true);
	painter.setPen(pen);

	if (_type == X) {
		if (_ticks) for (int i = 0; i < _ticks->size(); ++i) {
			const GraphWidget::Tick& t(_ticks->at(i));
			QString l = _locale.toString(t.uval);
			QRect  ts = fm.tightBoundingRect(l);

			painter.drawLine(t.pos, AXIS_TICK/2, t.pos, -AXIS_TICK/2);
			painter.drawText(t.pos - (ts.width()/2),
			                 ts.height() + AXIS_TICK/2 + AXIS_PADDING, l);
		}

		painter.drawText(_len/2 - _labelBB.width()/2,
			_labelBB.height() + th - 2*fm.descent()
			                  + AXIS_TICK/2 + 2*AXIS_PADDING,
			_label);

	} else {
		int mxw = 0;
		if (_ticks) for (int i = 0; i < _ticks->size(); ++i) {
			const GraphWidget::Tick& t(_ticks->at(i));
			QString l = _locale.toString(t.uval);
			QRect  ts = fm.tightBoundingRect(l);

			mxw = qMax(mxw, fm.width(l));
			painter.drawLine(AXIS_TICK/2, -t.pos, -AXIS_TICK/2, -t.pos);
			painter.drawText(-(ts.width() + AXIS_PADDING + AXIS_TICK/2),
			                 -t.pos + (ts.height()/2), l);
		}

		painter.rotate(-90);
		painter.drawText(_len/2 - _labelBB.width()/2,
			-(mxw + 2*AXIS_PADDING + AXIS_TICK/2), _label);
		painter.rotate(90);
	}

/*	painter.resetTransform();
	painter.setPen(Qt::red);
	painter.drawRect(rect());
	painter.drawLine(rect().topLeft(), rect().bottomRight());*/
}
