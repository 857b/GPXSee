#ifndef _COLORSCALES_H
#define _COLORSCALES_H

#include <QColor>
#include <QList>

#include "data/track.h"
#include "common/range.h"

class ColorScales {
public:
	virtual ~ColorScales() {};

	struct entry {
		ChanTy ty;
		RangeF range;
		
		entry() : range(RangeF::bottom()) {}
		entry(ChanTy ty, RangeF range) : ty(ty), range(range) {}
		static entry None() {return entry();}

		QColor colorAt(QColor base, qreal v);
		entry  color(ColorScales* css) const;
		void normalize();

		bool operator==(const entry& e) const
			{return ty==e.ty && range==e.range;}
	};

	void addEntry(const entry& e);
	void removeEntry(const entry& e);

	entry colorFor(const entry& e) const;

private:
	void updateTy(ChanTy ct);

	QList<entry> _entries;
	QMap<ChanTy, RangeF> _ranges;
};

#endif
