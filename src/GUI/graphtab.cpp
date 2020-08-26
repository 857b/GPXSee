#include "graphtab.h"

#include <QLocale>
#include <limits>
#include <cmath>

#include "data/data.h"
#include "tooltip.h"
#include "format.h"

// GraphSet

GraphSet::GraphSet(Track* parent)
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
	  _graph(graph) {}

void GraphItem1::finalize()
{
	updateG();
}

QString GraphItem1::info() const
{
	ToolTip tt;
	
	QString name(track().name());
	if (name.isEmpty())
		name = graph().label();
	tt.setTitle(name);

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
				const Track::Channel c(c0->filter(fltrWindow));

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
				const Track::Channel  c(c0->filter(fltrWindow));
		
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
				Track::filterWindow(graph().chanTy()));
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

QList<int> GraphTab1::channelsOfTrack(const Track& t)
{
	return t.findChannels(_ty);
}

GraphItem1* GraphTab1::makeTrackItem(GraphSet* set, int chId,
										const GraphItem1::Style& st)
{
	return new GraphItem1(set, chId, graphType(), st, this);
}

QList<GraphItem*> GraphTab1::loadGraphSet(Track& t, const QColor &color)
{
	QList<GraphItem*> rt;
	GraphSet* set = new GraphSet(&t);
	const QList<int> chan(channelsOfTrack(t));

	for (int i = 0; i < chan.size(); ++i) {
		if (!t.hasData(chan.at(i))) continue;
		GraphItem1 *gi = makeTrackItem(set, chan.at(i),
						GraphItem1::Style(_width, color, Qt::SolidLine));
		gi->setUnits(units());
		gi->finalize();
		
		if (_showTracks) addGraph(gi);

		rt.append(gi);
	}

	_sets.insert(set);
	connect(set, SIGNAL(destroyed()), this, SLOT(setDestroyed()));

	return rt;
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

	onGSetChange();
}

QList<QList<GraphItem*> > GraphTab1::loadData(Data &data)
{
	QList<QList<GraphItem*> > graphs;

	for (int i = 0; i < data.tracks().count(); i++) {
		Track& track(*data.tracks().at(i));
		QColor color(_palette.nextColor());
		graphs.append(loadGraphSet(track, color));
	}

	onGSetChange();

	return graphs;
}

QString GraphTab1::label() const
{
	return chanTyName(_ty);
}

void GraphTab1::onGSetChange()
{
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
	onGSetChange();
}

void GraphTab1::setUnits(Units units)
{
	GraphView::setUnits(units);
	
	updateInfoKeys();
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
