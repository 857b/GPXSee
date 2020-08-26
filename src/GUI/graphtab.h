#ifndef GRAPHTAB_H
#define GRAPHTAB_H

#include <QList>
#include "graphview.h"
#include "units.h"
#include "timetype.h"
#include "graphitem.h"
#include "data/track.h"
#include "tooltip.h"

class GraphTab;
class GraphTab1;
class GraphSet;
class GraphItem1;

class Data;
class GraphItem;



class GraphTab : public GraphView
{
	Q_OBJECT

public:
	GraphTab(QWidget *parent = 0) : GraphView(parent) {}
	virtual ~GraphTab() {}

	virtual QString label() const = 0;
	virtual QList<QList<GraphItem*> > loadData(Data &data) = 0;
	virtual void clear() {GraphView::clear();}
	virtual void setUnits(enum Units units) {GraphView::setUnits(units);}
	virtual void setGraphType(GraphType type) {GraphView::setGraphType(type);}
	virtual void setTimeType(enum TimeType type) {Q_UNUSED(type)}
	virtual void showTracks(bool show) {Q_UNUSED(show)}
	virtual void showRoutes(bool show) {Q_UNUSED(show)}
};

// 1

class GraphSet : public QObject
{
	Q_OBJECT

public:
	GraphSet(Track* parent);
	Track& track() const {return (Track&)*parent();}
	
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
	Track&        track() const {return set().track();}
	GraphTab1&    graph() const {return *_graph;}
	
	virtual QString info() const;
	virtual bool hasTime() const;
	virtual qreal yAtX(qreal x) const;
	virtual qreal distanceAtTime(qreal time) const;

	virtual GraphItem *secondaryGraph() const {return NULL;}

	qreal trackTime() const;
	int chanId() const {return _chId;}

protected:
	virtual void updatePath();
	virtual void updateBounds();

	virtual void makeTooltip(ToolTip& tt) const;

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
	QList<QList<GraphItem*> > loadData(Data &data);
	void clear();
	void setUnits(Units units);
	TimeType timeType() const {return _timeType;}
	void setTimeType(TimeType type);
	void showTracks(bool show);
	ChanTy chanTy() const {return _ty;}

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
	QList<GraphItem*> loadGraphSet(Track& t, const QColor &color);
private slots:
	void setDestroyed();

protected:
	virtual QList<int> channelsOfTrack(const Track& t);

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
