/*
 * WindowedFilter.h
 *
 *  Created on: Jul 3, 2012
 *      Author: nerd256
 */

#ifndef WINDOWEDFILTER_H_
#define WINDOWEDFILTER_H_

class WindowedFilter : public AbstractFilter {
public:
	WindowedFilter(double winsize = 1, double wininter = 1, bool need_copy = true);
	void setWindowSize(double winsize);
	void setWindowInterval(double wininter);
	virtual ~WindowedFilter();

	virtual void filter_window(TimeSeries * input, TimeSeries * output, int start, int end);
	virtual void filter_into(TimeSeries * in, TimeSeries * out);

protected:
	double window_size;
	double window_interval;
};

#endif /* WINDOWEDFILTER_H_ */
