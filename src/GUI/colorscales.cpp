#include "colorscales.h"

#include <Qt>

static inline QColor color_i(QColor base, qreal t)
{
	return QColor::fromRgbF(
		t * base.redF(),
		t * base.greenF(),
		t * base.blueF());
}

QColor ColorScales::scale::at(QColor base, qreal v)
{
	switch (ty) {
		case Base:
			return base;
		case Range:
			return color_i(base,
					(qMin(range.max, qMax(range.min, v)) - range.min)
				/ (range.max - range.min));
		case Sign:
			return v > 0
			  ? color_i(QColor(Qt::green), v / abs)
			  : color_i(QColor(Qt::red),  -v / abs);
	}
	return Base;
}

void ColorScales::scale::gradient(QGradient& out,
		QColor base, qreal v0, qreal v1)
{
	out.setStops(QGradientStops());
	out.setColorAt(0., at(base, v0));
	switch (ty) {
		case Sign:
			if (v0 * v1 < 0)
				out.setColorAt(v0 / (v0 - v1), Qt::black);
			break;
		case Base: case Range: break;
	}
	out.setColorAt(1., at(base, v1));
}

ColorScales::scale ColorScales::entry::color(ColorScales* css) const {
	entry e = css ? css->entryFor(*this) : *this;
	scale rt;
	if (e.range.size() <= 0)
		rt.ty = scale::Base;
	else if (e.ty == CTvSpeed) {
		rt.ty  = scale::Sign;
		rt.abs = qMax(e.range.max(), -e.range.min());
	} else {
		rt.ty = scale::Range;
		rt.range.min = e.range.min();
		rt.range.max = e.range.max();
	}
	return rt;
}

void ColorScales::addEntry(const entry& e)
{
	_entries.append(e);
	updateTy(e.ty);
}

void ColorScales::removeEntry(const entry& e)
{
	_entries.removeOne(e);
	updateTy(e.ty);
}

ColorScales::entry ColorScales::entryFor(const entry& e) const
{
	QMap<ChanTy, RangeF>::const_iterator irg = _ranges.find(e.ty);
	return irg == _ranges.end() ? e : entry(e.ty, irg.value());
}

void ColorScales::updateTy(ChanTy ct)
{
	RangeF rg = RangeF::bottom();
	for (int i = 0; i < _entries.size(); ++i)
		if (_entries.at(i).ty == ct)
			rg |= _entries.at(i).range;
	if (rg.isValid())
		_ranges.insert(ct, rg);
	else
		_ranges.remove(ct);
}
