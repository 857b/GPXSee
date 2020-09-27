#ifndef GRAPHTAB_H
#define GRAPHTAB_H

#include <QList>
#include <QPointer>
#include <QAction>
#include "graphview.h"
#include "units.h"
#include "timetype.h"
#include "graphitem.h"
#include "gdata.h"
#include "tooltip.h"

class GraphTab;
class GraphTab1;
class GraphSet;
class GraphItem1;


class GraphTab : public GraphView
{
	Q_OBJECT

public:
	GraphTab(QWidget *parent = 0) : GraphView(parent) {}
	virtual ~GraphTab() {}

	virtual QString label() const = 0;

	/* return:
	 *   a list of size data.routes().size() such that
	 *   ret[i] is the item associated with data.routes()[i] or NULL
	 */
	virtual QList<GraphItem* > loadData(GData &data) = 0;
	virtual void clear() {GraphView::clear();}
	virtual void setUnits(enum Units units) {GraphView::setUnits(units);}
	virtual void setGraphType(GraphType type) {GraphView::setGraphType(type);}
	virtual void setTimeType(enum TimeType type) {Q_UNUSED(type)}
	virtual void showTracks(bool show) {Q_UNUSED(show)}
	virtual void showRoutes(bool show) {Q_UNUSED(show)}
};

// 1

// All children are GraphItem1
class GraphSet : public QObject
{
	Q_OBJECT

public:
	GraphSet(GTrack* parent);
	GTrack& track() const {return (GTrack&)*parent();}
	
	bool isValid(GraphType ty) const;
};



class GraphItem1 : public GraphItem
{
	Q_OBJECT

public:
	struct Style {
		int           width;
		const QColor& color;
		Qt::PenStyle  pen;

		Style(int width, const QColor& color, Qt::PenStyle pen)
			: width(width), color(color), pen(pen) {}
	};

	GraphItem1(GraphSet* set, int chId, GraphType type,
			const Style& style,
			GraphTab1* graph, QGraphicsItem *iParent = 0);

	virtual void finalize();

	GraphSet&     set()   const {return (GraphSet&)*parent();}
	GTrack&       track() const {return set().track();}
	const Track::ChannelDescr& chanDescr() const 
		{return track().chanDescr().at(_chId);}
	GraphTab1&    graph() const {return *_graph;}

	virtual QString   name(bool full = false) const;
	virtual QString   info() const;
	virtual bool      hasTime() const;
	virtual qreal     yAtX(qreal x) const;
	virtual qreal     distanceAtTime(qreal time) const;
	virtual QDateTime dateAtX(qreal x) const;

	virtual GraphItem *secondaryGraph() const {return NULL;}

	qreal trackTime() const;
	int chanId() const {return _chId;}

private slots:
	void mainGraphAction(QAction* action);
	void channelOnPathAction(bool display);

protected:
	virtual void updatePath();
	virtual void updateBounds();

	virtual void makeTooltip(ToolTip& tt) const;

	void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
private:
	int        _chId;
	GraphTab1* _graph;
};



class GraphTab1 : public GraphTab
{
	Q_OBJECT

public:
	GraphTab1(ChanTy ty, QWidget *parent = 0);
	~GraphTab1();

	QString label() const;
	QList<GraphItem* > loadData(GData &data);
	void clear();
	void setUnits(Units units);
	void setGraphType(GraphType type);
	TimeType timeType() const {return _timeType;}
	void setTimeType(TimeType type);
	void showTracks(bool show);
	ChanTy chanTy() const {return _ty;}

	std::pair<GraphItem*, GraphItem*> mainGraphs();
	void setMainGraph(GraphItem1* item, bool set = true,
			bool secondary = false);
	GraphItem1* getMainGraph(bool secondary = false);

	template<typename F>
	void iterTrackItems(F& f) const {
		for (QSet<GraphSet*>::const_iterator s = _sets.begin();
				s != _sets.end(); ++s) {
			const QObjectList& gs = (**s).children();
			for (int i = 0; i < gs.size(); ++i)
				f(static_cast<const GraphItem1&>(*gs.at(i)));
		}
	}

	template<typename I>
	struct sum_f {
		qreal (I::*get)() const;
		qreal s;
		
		sum_f(qreal (I::*get)() const) : get(get), s(0) {}
		void operator()(const GraphItem1& itm) {
			s += (static_cast<const I&>(itm).*get)();
		}
	};

	template<typename I>
	qreal sumTrackItems(qreal (I::*get)() const) const {
		sum_f<I> f(get);
		iterTrackItems(f);
		return f.s;
	}

private:
	void loadGraphSet(GTrack& t, const QColor &color);
private slots:
	void setDestroyed();

protected:
	virtual QList<int> channelsOfTrack(const GTrack& t);

	qreal max() const {return bounds().bottom();}
	qreal min() const {return bounds().top();}
	qreal tracksTime() const;

	void onGSetChange();

	virtual void updateInfoKeys();
	virtual void updateTracksInfos();
	virtual GraphItem1* makeTrackItem(GraphSet* set, int chId,
										const GraphItem1::Style& st);

	void addInfo(const QString &key, qreal val, const Unit& u,
				 const Unit::Fmt& fmt = Unit::Fmt());

	ChanTy   _ty;

	TimeType _timeType;

	bool     _showTracks;
	QSet<GraphSet*> _sets;
	QPointer<GraphItem1> _mainGraphs[2];
};

// 2

class GraphItem2 : public GraphItem1
{
public:
	GraphItem2(GraphSet* set, int chId, GraphType type,
						const Style& style, GraphTab1* graph);

	qreal sum() const {return _sum;}
	qreal tsum() const {return _tsum;}
	qreal avg() const {return _tsum / trackTime();}

protected:
	qreal _sum, _tsum;
};

class GraphTab2 : public GraphTab1
{
public:
	GraphTab2(ChanTy t, QWidget *p = 0) : GraphTab1(t, p) {}

protected:
	GraphItem1* makeTrackItem(GraphSet* set, int chId,
									const GraphItem1::Style& st);
	qreal sum();
	qreal tsum();
	qreal avg();
};

#endif // GRAPHTAB_H
