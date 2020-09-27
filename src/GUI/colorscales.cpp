#include "colorscales.h"

#include <Qt>

static inline QColor color_i(QColor base, qreal t)
{
	return QColor::fromRgbF(
		t * base.redF(),
		t * base.greenF(),
		t * base.blueF());
}

QColor ColorScales::entry::colorAt(QColor base, qreal v)
{
	if (!range.isValid()) return base;
	v = range.into(v);
	if (ty == CTvSpeed)
		return v > 0
		  ? color_i(QColor(Qt::green), v / range.max())
		  : color_i(QColor(Qt::red),   v / range.min());
	else
		return color_i(base, (v - range.min()) / range.size());
}

ColorScales::entry ColorScales::entry::color(ColorScales* css) const {
	entry e = css ? css->colorFor(*this) : *this;
	e.normalize();
	return e;
}

void ColorScales::entry::normalize() {
	if (ty == CTvSpeed) {
		range.setMax(qMax(range.max(), -range.min()));
		range.setMin(-range.max());
	}
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

ColorScales::entry ColorScales::colorFor(const entry& e) const
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
