/*
 * AbstractFIlter.h
 *
 *  Created on: Jul 3, 2012
 *      Author: nerd256
 */

#ifndef ABSTRACTFILTER_H_
#define ABSTRACTFILTER_H_

class AbstractFilter {
public:
	AbstractFilter(bool need_copy = true);
	~AbstractFilter();

	TimeSeries * filter(TimeSeries * ts);
	TimeSeries * filter_iterate(TimeSeries * ts, bool (*update_func)(AbstractFilter *, int iter));

	virtual void filter_into(TimeSeries * in, TimeSeries * out);

	bool needs_copy;
};

#endif /* ABSTRACTFILTER_H_ */
