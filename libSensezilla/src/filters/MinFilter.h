/*
 * MinFilter.h
 *
 *  Created on: Jul 3, 2012
 *      Author: nerd256
 */

#ifndef MINFILTER_H_
#define MINFILTER_H_

class MinFilter : public WindowedFilter {
public:
	virtual void filter_window(TimeSeries * input, TimeSeries * output, int start, int end);
};

#endif /* MINFILTER_H_ */
