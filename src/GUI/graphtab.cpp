#include "graphtab.h"

#include <QContextMenuEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QWidgetAction>
#include <QApplication>
#include <limits>
#include <cmath>

#include "data/data.h"
#include "data/compute.h"
#include "common/util.h"
#include "tooltip.h"
#include "format.h"

#define SELECT_MENU 1

// GraphSet

GraphSet::GraphSet(GTrack* parent)
	: QObject(parent) {}

bool GraphSet::isValid(GraphType ty) const
{
	return !children().isEmpty() && (ty == Distance || track().hasTime());
}

// GraphItem1

GraphItem1::GraphItem1(GraphSet* set, int chId, GraphType type,
			const Style& st,
			GraphTab1* graph, QGraphicsItem *iParent)
	: GraphItem(set, type, st.width, st.color, st.pen, iParent),
	  _chId(chId),
	  _graph(graph) {
	setAcceptedMouseButtons(Qt::LeftButton);
}

void GraphItem1::finalize()
{
	updateG();
}

QString GraphItem1::name(bool full) const
{
	QString name(track().name());
	if (name.isEmpty())
		name = graph().label();

	QString cName(chanDescr().name(track(), full));
	if (!cName.isEmpty())
		name += QString(" - ") + cName;
	return name;
}

QString GraphItem1::info() const
{
	ToolTip tt;
	tt.setTitle(name());
	makeTooltip(tt);
	return tt.toString();
}

void GraphItem1::makeTooltip(ToolTip& tt) const
{
	Q_UNUSED(tt);
}

bool GraphItem1::hasTime() const
{
	return track().hasTime();
}

qreal GraphItem1::yAtX(qreal x) const
{
	const Track& tk(track());
	Track::ChannelPoint p = gType() == Time ? tk.pointAtTime(x)
	                                        : tk.pointAtDistance(x);
	const Track::Channel* c;
	if (!p || !(c = tk.segments().at(p.seg_num).findChannel(_chId)))
		return NAN;
	return p.interpol(*c);
}

qreal GraphItem1::distanceAtTime(qreal time) const
{
	return track().distanceAtTime(time);
}

QDateTime GraphItem1::dateAtX(qreal x) const
{
	const Track& tk(track());
	Track::ChannelPoint p = gType() == Time ? tk.pointAtTime(x)
	                                        : tk.pointAtDistance(x);
	if (!p) return QDateTime();
	const Track::Segment& s(tk.segments().at(p.seg_num));
	if (!s.hasTime()) return QDateTime();
	return p.interpol(s.time);
}
	
template<typename F>
static inline void iterGraph(F& f, GraphType gt,
		const Track& tk, int cid, int fltrWindow = 1)
{
	switch (gt) {
		case Time:
			for (int i_s = 0; i_s < tk.segments().count(); ++i_s) {
				const Track::Segment& s(tk.segments().at(i_s));
				if (!s.hasTime()) continue;
				const Track::Channel* c0(s.findChannel(cid));
				if (!c0) continue;
				const Track::Channel c(
						Compute::filter(*c0, fltrWindow, s.outliers));

				bool beginSeg = true;
				for (int i_p = 0; i_p < c.size(); ++i_p) {
					if (s.outliers.contains(i_p)) continue;
					qreal v(c.at(i_p));
					if (std::isnan(v)) continue;
					qreal t(s.timeAt(i_p));
					f(t, v, beginSeg);
					beginSeg = false;
				}
			}
		break;
		case Distance: {
			int dChan(tk.distChan());
			for (int i_s = 0; i_s < tk.segments().count(); ++i_s) {
				const Track::Segment& s(tk.segments().at(i_s));
				const Track::Channel* c0(s.findChannel(cid));
				if (!c0) continue;
				const Track::Channel& c_d(s[dChan]);
				const Track::Channel  c(
						Compute::filter(*c0, fltrWindow, s.outliers));
		
				bool beginSeg = true;
				for (int i_p = 0; i_p < c.size(); ++i_p) {
					if (s.outliers.contains(i_p)) continue;
					qreal v(c.at(i_p));
					if (std::isnan(v)) continue;
					qreal d(s.dist0 + c_d.at(i_p));
					f(d, v, beginSeg);
					beginSeg = false;
				}
			}
		}break;
	}
}

struct PathMaker
{
	QPainterPath& path;
	PathMaker(QPainterPath& p) : path(p) {}
	void operator()(qreal x, qreal y, bool bgSg) {
		if (bgSg)
			path.moveTo(x, y);
		else
			path.lineTo(x, y);
	}
};
void GraphItem1::updatePath()
{ 
	_path = QPainterPath();
	PathMaker mk(_path);
	iterGraph(mk, gType(), track(), _chId,
				Compute::filterWindow(track().chanDescr().at(_chId)));
}

struct BoundComputer
{
	qreal bottom, top, left, right;

	BoundComputer()
		: bottom(-std::numeric_limits<qreal>::infinity()),
		  top(std::numeric_limits<qreal>::infinity()),
		  left(std::numeric_limits<qreal>::infinity()),
		  right(-std::numeric_limits<qreal>::infinity())
	{}

	void operator()(qreal x, qreal y, bool sg) {
		Q_UNUSED(sg);
		left   = qMin(left,   x);
		right  = qMax(right,  x);
		top    = qMin(top,    y);
		bottom = qMax(bottom, y);
	}

	operator QRectF() const {
		return left < right && top < bottom
			? QRectF(QPointF(left, top), QPointF(right, bottom))
			: QRectF();
	}
};

void GraphItem1::updateBounds()
{
	if (gType() == Time && !hasTime())
		_bounds = QRectF();
	else {
		BoundComputer bc;
		iterGraph(bc, gType(), track(), _chId);
		_bounds = (QRectF)bc;
	}
}

qreal GraphItem1::trackTime() const
{
	const QList<Track::Segment>& sgs(track().segments());
	qreal rt = 0;
	for (int i = 0; i < sgs.count(); ++i) {
		const Track::Segment& sg(sgs.at(i));
		if (!sg.hasTime() || !sg.findChannel(_chId)) continue;
		switch(graph().timeType()) {
			case Total:
				rt += sg.totalTime();
				break;
			case Moving:
				rt += sg.movingTime();
				break;
		}
	}
	return rt;
}

void GraphItem1::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	event->accept();
	QMenu *menu = new QMenu;

	QLabel* title = new QLabel("<i>" + qstring_toHtmlEscaped(name()) + "</i>");
	title->setTextFormat(Qt::RichText);
	title->setMargin(4);
	title->setAlignment(Qt::AlignLeft);
	QWidgetAction* title_act = new QWidgetAction(menu);
	title_act->setDefaultWidget(title);
	menu->addAction(title_act);
	menu->addSeparator();
	{
		QAction* hideAct = menu->addAction(tr("Hide"));
		connect(hideAct, SIGNAL(triggered()),
				this,    SLOT(hideAction()));

		QAction* copAct = menu->addAction(tr("Display on path"));
		copAct->setCheckable(true);
		copAct->setChecked(track().displayedChannel() == chanId());
		connect(copAct, SIGNAL(triggered(bool)),
				this,   SLOT(channelOnPathAction(bool)));
	}
	menu->addSeparator();

	QActionGroup* mainGmenu = new QActionGroup(menu);
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	mainGmenu->setExclusive(false);
#else
	mainGmenu->setExclusionPolicy(
			QActionGroup::ExclusionPolicy::ExclusiveOptional);
#endif
	QString act_name[2] = {tr("Main graph"), tr("Secondary graph")};
	for (int i = 0; i < 2; ++i) {
		bool sec = i;
		QAction *act = menu->addAction(act_name[i]);
		bool   isCur = graph().getMainGraph(sec) == this;
		act->setData((int)((sec ? 0b10 : 0) | (isCur ? 1 : 0)));
		act->setCheckable(true);
		act->setChecked(isCur);
		mainGmenu->addAction(act);
	}
	connect(mainGmenu, SIGNAL(triggered(QAction*)),
			this,      SLOT(mainGraphAction(QAction*)));

	setSelected(SELECT_MENU, true);
	connect(menu, SIGNAL(aboutToHide()), this, SLOT(menuClose()));
	menu->popup(event->screenPos());
}

void GraphItem1::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	event->accept();
	if (QApplication::keyboardModifiers() & Qt::ControlModifier)
		channelOnPathAction(track().displayedChannel() != chanId());
}

//TODO: option
void GraphItem1::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	event->accept();
	graph().setMainGraph(this, true);
}

void GraphItem1::menuClose()
{
	setSelected(SELECT_MENU, false);
}

void GraphItem1::mainGraphAction(QAction* action)
{
	int  dt = action->data().toInt();
	bool set = !(dt & 0b1), secondary = dt & 0b10;
	graph().setMainGraph(this, set, secondary);
}

void GraphItem1::channelOnPathAction(bool display)
{
	track().setDispChannel(display ? chanId() : ~0);
}

void GraphItem1::hideAction()
{
	graph().setMainGraph(this, false);
	hide();
}

// GraphTab1

GraphTab1::GraphTab1(ChanTy ty, QWidget* parent)
	: GraphTab(parent),
	  _ty(ty),
	  _timeType(Total),
	  _showTracks(true)
{
	setYLabel(label());
}

GraphTab1::~GraphTab1() {
	clear();
}

void GraphTab1::updateInfoKeys()
{
	clearInfo();
	if (_showTracks)
		updateTracksInfos();
}

void GraphTab1::updateTracksInfos() {}

void GraphTab1::addInfo(const QString &key, qreal val,
						const Unit& u, const Unit::Fmt& fmt)
{
	if (std::isfinite(val))
		GraphView::addInfo(key, u.format(val, fmt));
}

struct all_visible_f {
	bool av; all_visible_f():av(true){}
	void operator()(const GraphItem1& i) {av &= i.isVisible();}
};
void GraphTab1::contextMenuEvent(QContextMenuEvent *event)
{
	event->accept();
	QMenu *menu = new QMenu;
	QAction* showAct = menu->addAction(tr("Show hidden graphs"));
	all_visible_f f;
	iterTrackItems(f);
	if (f.av) showAct->setEnabled(false);
	connect(showAct, SIGNAL(triggered()),
			this,    SLOT(showHiddenGraphs()));
	menu->popup(event->globalPos());
}

QList<int> GraphTab1::channelsOfTrack(const GTrack& t)
{
	return t.findChannels(_ty);
}

GraphItem1* GraphTab1::makeTrackItem(GraphSet* set, int chId,
										const GraphItem1::Style& st)
{
	return new GraphItem1(set, chId, graphType(), st, this);
}

static const unsigned graphStylesCount = 3;
static const Qt::PenStyle graphStyles[3] = {
		Qt::SolidLine, Qt::DashLine, Qt::DotLine
	};
void GraphTab1::loadGraphSet(GTrack& t, const QColor &color)
{
	GraphSet* set = new GraphSet(&t);
	const QList<int> chan(channelsOfTrack(t));
	unsigned styleIdx = 0;

	for (int i = 0; i < chan.size(); ++i) {
		if (!t.hasData(chan.at(i))) continue;
		GraphItem1 *gi = makeTrackItem(set, chan.at(i),
						GraphItem1::Style(_width, color,
							graphStyles[styleIdx++ % graphStylesCount]));
		gi->setUnits(units());
		gi->finalize();
		
		if (_showTracks) addGraph(gi);
	}

	t.addGraphSet(set);
	_sets.insert(set);
	connect(set, SIGNAL(destroyed()), this, SLOT(setDestroyed()));
}

void GraphTab1::setDestroyed()
{
	GraphSet* set(static_cast<GraphSet*>(QObject::sender()));
	const QObjectList& gs = set->children();
	for (int i = 0; i < gs.size(); ++i) {
		GraphItem1* g(static_cast<GraphItem1*>(gs[i]));
		removeGraph(g);
	}
	_sets.remove(set);
	for (int i = 0; i < 2; ++i)
		if (!_mainGraphs[i].isNull() && &_mainGraphs[i]->set() == set)
			_mainGraphs[i] = NULL;

	onGSetChange();
}

struct show_f{void operator()(GraphItem1& i){i.show();}};
void GraphTab1::showHiddenGraphs()
{
	show_f f;
	iterTrackItems(f);
}

QList<GraphItem*> GraphTab1::loadData(GData &data)
{
	for (int i = 0; i < data.tracks().count(); i++) {
		GTrack& track(*data.tracks().at(i));
		QColor color(_palette.nextColor());
		loadGraphSet(track, color);
	}

	onGSetChange();

	QList<GraphItem*> ret;
	ret.reserve(data.routes().size());
	for (int i = 0; i < data.routes().size(); ++i)
		ret.append(NULL);
	return ret;
}

QString GraphTab1::label() const
{
	return chanTyName(_ty);
}

void GraphTab1::onGSetChange()
{
	onMainGraphChanged();
	updateInfoKeys();
	redraw();
}

void GraphTab1::clear()
{
	GraphTab::clear();
	for (QSet<GraphSet*>::iterator s = _sets.begin();
			s != _sets.end(); ++s) {
		GraphSet* set(*s);
		disconnect(set, SIGNAL(destroyed()), this, SLOT(setDestroyed()));
		set->deleteLater();
	}
	_sets.clear();
	_mainGraphs[0] = _mainGraphs[1] = NULL;
	onGSetChange();
}

void GraphTab1::setUnits(Units units)
{
	GraphView::setUnits(units);
	
	updateInfoKeys();
}

void GraphTab1::setGraphType(GraphType type)
{
	GraphTab::setGraphType(type);
	onGSetChange();
}

void GraphTab1::setTimeType(enum TimeType type)
{
	_timeType = type;

	updateInfoKeys();
	redraw();
}

void GraphTab1::showTracks(bool show)
{
	if (show == _showTracks) return;

	_showTracks = show;

	for (QSet<GraphSet*>::iterator s = _sets.begin();
			s != _sets.end(); ++s) {
		const QObjectList& gs = (**s).children();
		for (int i = 0; i < gs.size(); ++i) {
			GraphItem1* g(static_cast<GraphItem1*>(gs[i]));
			if (show)
				addGraph(g);
			else
				removeGraph(g);
		}
	}

	updateInfoKeys();
	redraw();
}

qreal GraphTab1::tracksTime() const
{
	return sumTrackItems(&GraphItem1::trackTime);
}

std::pair<GraphItem*, GraphItem*> GraphTab1::mainGraphs()
{
	if (_mainGraphs[0].isNull())
		return std::pair<GraphItem*, GraphItem*>(NULL, NULL);
	return std::pair<GraphItem*, GraphItem*>
					(_mainGraphs[0], _mainGraphs[1]);
}

void GraphTab1::setMainGraph(GraphItem1* item, bool set, bool secondary)
{
	if (set) {
		int i = secondary ? 1 : 0;
		if (_mainGraphs[i^1] == item)
			std::swap(_mainGraphs[0], _mainGraphs[1]);
		else if (!secondary && _mainGraphs[i] != item) {
			_mainGraphs[1] = _mainGraphs[0];
			_mainGraphs[0] = item;
		} else
			_mainGraphs[i] = item;
	} else
		for (int i = 0; i < 2; ++i)
			if (_mainGraphs[i] == item)
				_mainGraphs[i] = NULL;
	onMainGraphChanged();
}

GraphItem1* GraphTab1::getMainGraph(bool secondary)
{
	return _mainGraphs[secondary ? 1 : 0];
}

// GraphTab2

GraphItem2::GraphItem2(GraphSet* set, int chId, GraphType type,
						const Style& style, GraphTab1* graph)
	: GraphItem1(set, chId, type, style, graph)
{
	const Track::ChannelDescr& cd(track().chanDescr().at(chId));
	_sum  = cd.sum (track(), chId);
	_tsum = cd.tsum(track(), chId);
}

GraphItem1* GraphTab2::makeTrackItem(GraphSet* set, int chId,
								const GraphItem1::Style& st)
{
	return new GraphItem2(set, chId, graphType(), st, this);
}

qreal GraphTab2::sum()
{
	return sumTrackItems(&GraphItem2::sum);
}

qreal GraphTab2::tsum()
{
	return sumTrackItems(&GraphItem2::tsum);
}

qreal GraphTab2::avg()
{
	return sum() / tracksTime();
}
