/*
 * SpikeFilter.cpp
 *
 *  Created on: Jul 3, 2012
 *      Author: nerd256
 */

#include "all.h"

void SpikeFilter::filter_window(TimeSeries * input, TimeSeries * output, int start, int end) {
	int mini=start, maxi=start;
	double minv = input->v[start], maxv = input->v[start];

	for ( int c = start; c <= end; c++ ) {
		if ( input->v[c] < minv) {
			mini = c;
			minv = input->v[c];
		}
		if ( input->v[c] > maxv) {
			maxi = c;
			maxv = input->v[c];
		}
	}

	int fspot = 0;
	if(minv <= input->v[start] - spike_thresh && minv <= input->v[end] - spike_thresh) { // neg spike
		fspot = mini;
	} else if(maxv >= input->v[start] + spike_thresh && maxv >= input->v[end] + spike_thresh) { // pos spike
		fspot = maxi;
	} else { // no spike, do nothing
		return;
	}
	/*if (input->t[start]-input->t[0] >= 46000 && input->t[start]-input->t[0] <= 47000) {
			printf("\t%d %.2f %.2f\n\t%d %.2f %.2f\n\t%d %d\n\t%d %.2f %.2f\n\t%p\n",start,input->t[start],input->v[start],
					end,input->t[end],input->v[end],
					mini,maxi,
					fspot,input->t[fspot],input->v[fspot],
					output);
	}*/
	for ( int c = start; c <= fspot; c++ ) {
		output->v[c] = input->v[start];
	}
	for ( int c = fspot + 1; c <= end; c++) {
		output->v[c] = input->v[end];
	}

}

void SpikeFilter::setSpikeThreshold(double d) {
	spike_thresh = d;
}

SpikeFilter::SpikeFilter(double skp_thresh) {
	setSpikeThreshold(skp_thresh);
}
SpikeFilter::~SpikeFilter() {}

