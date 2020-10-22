#ifndef _COLORSCALES_H
#define _COLORSCALES_H

#include <QColor>
#include <QList>
#include <QGradient>

#include "data/track.h"
#include "common/range.h"

class ColorScales {
public:
	virtual ~ColorScales() {};

	class scale {
		friend ColorScales;
		scale() {}
	public:
		enum {
			Base, Range, Sign
		} ty;
		union {
			struct {qreal min, max;}
				   range; // size > 0
			qreal  abs;   // > 0
		};

		QColor at(QColor base, qreal v);
		void gradient(QGradient& out, QColor base, qreal v0, qreal v1);
	};

	struct entry {
		ChanTy ty;
		RangeF range;

		entry() : range(RangeF::bottom()) {}
		entry(ChanTy ty, RangeF range) : ty(ty), range(range) {}
		static entry None() {return entry();}

		scale  color(ColorScales* css) const;

		bool operator==(const entry& e) const
			{return ty==e.ty && range==e.range;}
	};

	void addEntry(const entry& e);
	void removeEntry(const entry& e);

	entry entryFor(const entry& e) const;

private:
	void updateTy(ChanTy ct);

	QList<entry> _entries;
	QMap<ChanTy, RangeF> _ranges;
};

#endif
