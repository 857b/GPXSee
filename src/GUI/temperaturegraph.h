#ifndef TEMPERATUREGRAPH_H
#define TEMPERATUREGRAPH_H

#include "graphtab.h"

class TemperatureGraphItem: public GraphItem2
{
	Q_OBJECT

public:
	TemperatureGraphItem(GraphSet* s, int c, GraphType t,
						const Style& y, GraphTab1* g)
		: GraphItem2(s, c, t, y, g) {}

protected:
	virtual void makeTooltip(ToolTip& tt) const;
};

class TemperatureGraph : public GraphTab2
{
	Q_OBJECT

public:
	TemperatureGraph(QWidget *parent = 0);

	void setUnits(enum Units units);

protected:
	void updateTracksInfos();
	GraphItem1* makeTrackItem(GraphSet* set, int chId,
									const GraphItem1::Style& st);

private:
	void setYUnits(Units units);
};

#endif // TEMPERATUREGRAPH_H
