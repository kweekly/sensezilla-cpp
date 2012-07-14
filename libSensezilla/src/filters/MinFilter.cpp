/*
 * MinFilter.cpp
 *
 *  Created on: Jul 3, 2012
 *      Author: nerd256
 */

#include "all.h"

void MinFilter::filter_window(TimeSeries * input, TimeSeries * output, int start, int end) {
	double minval = 9e99;
	for(int c = start; c <= end; c++) {
		if ( input->v[c] < minval )
			minval = input->v[c];
	}
	for ( int c = start; c <= end; c++ ) {
		output->v[c] = minval;
	}
}
