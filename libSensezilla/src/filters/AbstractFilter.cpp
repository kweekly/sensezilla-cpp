/*
 * AbstractFIlter.cpp
 *
 *  Created on: Jul 3, 2012
 *      Author: nerd256
 */

#include "all.h"

TimeSeries * AbstractFilter::filter(TimeSeries * ts) {
	TimeSeries * buf = ts->copy();
	filter_into(ts, buf);
	delete ts;
	return buf;
}

TimeSeries * AbstractFilter::filter_iterate(TimeSeries * ts, bool (*update_func)(AbstractFilter *, int iter)) {
	TimeSeries * buf = ts->copy();
	bool retval = true;
	int iter = 0;
	while(retval) {
		retval = update_func(this, iter);
		if ( !retval ) break;
		if ( iter > 0 && needs_copy ){
			for ( int c = 0; c < ts->t.size(); c++ )
				buf->v[c] = ts->v[c];
		}
		filter_into(ts, buf);
		TimeSeries * temp = buf;
		buf = ts;
		ts = temp;
		iter++;
	}
	delete buf;
	return ts;
}

void AbstractFilter::filter_into(TimeSeries * in, TimeSeries * out) {
}

AbstractFilter::AbstractFilter(bool need_copy) {
	needs_copy = need_copy;
}

AbstractFilter::~AbstractFilter() {}
