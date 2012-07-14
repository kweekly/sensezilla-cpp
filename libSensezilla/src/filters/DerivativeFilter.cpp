/*
 * DerivativeFilter.cpp
 *
 *  Created on: Jul 11, 2012
 *      Author: nerd256
 */

#include "all.h"

DerivativeFilter::DerivativeFilter(double winsize) : AbstractFilter(false) {
	setWindowSize(winsize);
}
void DerivativeFilter::setWindowSize(double w) {
	window_size = w;
}

DerivativeFilter::~DerivativeFilter() {}

void DerivativeFilter::filter_into(TimeSeries * in, TimeSeries * out) {
	// we actually can't tell the derivative for the first half-window of points
	int pc = 0;
	while ( in->t[pc] < window_size / 2 ) {
		out->v[pc] = 0;
		pc++;
	}

	while ( in->t[pc] <= in->t.back() - window_size/2 ) {
		int pw = 1;
		double sl = 0, sr = 0;
		while ( in->t[pc + pw] - in->t[pc - pw] < window_size ) {
			sl += in->v[pc - pw]; // sum of left side
			sr += in->v[pc + pw]; // sum of right side
			pw++;
		}
		out->v[pc] = (sr - sl) / (window_size/2);
		pc++;
	}

	// can't tell the derivative for the last half-window of points
	while ( pc < in->t.size() ) {
		out->v[pc] = 0;
		pc++;
	}
}
