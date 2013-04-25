/*
 * WindowedFilter.cpp
 *
 *  Created on: Jul 3, 2012
 *      Author: nerd256
 */

#include "all.h"

WindowedFilter::WindowedFilter(double winsize, double wininter, bool init_copy) : AbstractFilter(init_copy) {
	setWindowSize(winsize);
	setWindowInterval(wininter);
}

void WindowedFilter::setWindowSize(double winsize) {
	window_size = winsize;
}

void WindowedFilter::setWindowInterval(double wininter) {
	window_interval = wininter;
}

void WindowedFilter::filter_window(TimeSeries * input, TimeSeries * output, int start, int end) { }

void WindowedFilter::filter_into(TimeSeries * ts, TimeSeries * out) {
	// travel half the window size
	double tpos = window_size/2 + ts->t[0];
	int ipos = 0;

	while(ts->t[ipos++] < tpos);
	tpos = ts->t[ipos];
	size_t start, end;
	start = end = ipos;
	while(end < ts->t.size()) {
		start = end = ipos;
		while(start > 0 && tpos - ts->t[start] < window_size/2) { start--;}
		while(end < ts->t.size() && ts->t[end] - tpos < window_size/2) {end++;}
		filter_window(ts, out, start, end - 1);
		while(ts->t[ipos] <= ts->t[ts->t.size()-1] - window_size/2 && ts->t[ipos] < tpos + window_interval) ipos++;
		tpos = ts->t[ipos];
		//printf("%d %d %d %.2f\n",ipos,start, end, tpos);
	}
}

WindowedFilter::~WindowedFilter() {
}
