#include "range.h"

void RangeF::resize(qreal size)
{
	qreal adj = (size/2 - this->size()/2);

	_min -= adj;
	_max += adj;
}

RangeF RangeF::operator&(const RangeF &r) const
{
	if (isNull() || r.isNull())
		return RangeF();

	RangeF tmp(qMax(this->_min, r._min), qMin(this->_max, r._max));
	return tmp.isValid() ? tmp : RangeF();
}

RangeF operator*(qreal scale, const RangeF& r) {
	RangeF rt(scale * r._min, scale * r._max);
	if (scale < 0)
		std::swap(rt._min, rt._max);
	return rt;
}

RangeF operator+(qreal ofs,   const RangeF& r) {
	return RangeF(r._min + ofs, r._max + ofs);
}

#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const Range &range)
{
	dbg.nospace() << "Range(" << range.min() << ", " << range.max() << ")";
	return dbg.space();
}

QDebug operator<<(QDebug dbg, const RangeF &range)
{
	dbg.nospace() << "RangeF(" << range.min() << ", " << range.max() << ")";
	return dbg.space();
}
#endif // QT_NO_DEBUG
