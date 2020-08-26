#ifndef ELEVATIONGRAPH_H
#define ELEVATIONGRAPH_H

#include "graphtab.h"

class ElevationGraphItem_data
{
	Q_DECLARE_TR_FUNCTIONS(ElevationGraphItem)

public:
	ElevationGraphItem_data(const Graph& g);
	ElevationGraphItem_data(const Track& t, int chId);

	qreal ascent()  const {return _ascent;}
	qreal descent() const {return _descent;}

protected:
	void makeTooltip(ToolTip& tt, Unit u, qreal min, qreal max) const;

	qreal _ascent, _descent;
};

class ElevationGraphTItem : public GraphItem1,
							public ElevationGraphItem_data
{
	Q_OBJECT

public:
	ElevationGraphTItem(GraphSet* s, int c, GraphType t,
						const Style& y, GraphTab1* g)
		: GraphItem1(s, c, t, y, g),
		  ElevationGraphItem_data(track(), chanId()) {}

protected:
	virtual void makeTooltip(ToolTip& tt) const {
		return ElevationGraphItem_data::makeTooltip(tt, graph().yUnit(),
					GraphItem1::min(), GraphItem1::max());
	}
};

class ElevationGraphRItem : public GraphItem0,
							public ElevationGraphItem_data
{
	Q_OBJECT

public:
	ElevationGraphRItem(const Graph &g, GraphType t, int w,
					   const QColor &c, Qt::PenStyle y)
		: GraphItem0(g, t, w, c, y), ElevationGraphItem_data(g) {}

	QString info() const;
};

class ElevationGraph : public GraphTab1
{
	Q_OBJECT

public:
	ElevationGraph(QWidget *parent = 0);
	~ElevationGraph();

	QList<QList<GraphItem*> > loadData(Data &data);
	void clear();
	void setUnits(enum Units units);
	void showRoutes(bool show);

protected:
	void updateInfoKeys();
	GraphItem1* makeTrackItem(GraphSet* set, int chId,
									const GraphItem1::Style& st);

private:
	qreal ascent()  const;
	qreal descent() const;

	qreal trackAscent()  const;
	qreal trackDescent() const;

	void setYUnits(Units units);

	ElevationGraphRItem *loadRGraph(const Graph &graph,
									const QColor &color, bool primary);

	qreal _routeAscent, _routeDescent;

	bool _showRoutes;
	QList<ElevationGraphRItem *> _routes;
};

#endif // ELEVATIONGRAPH_H
