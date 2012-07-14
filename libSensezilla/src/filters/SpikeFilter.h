/*
 * SpikeFilter.h
 *
 *  Created on: Jul 3, 2012
 *      Author: nerd256
 */

#ifndef SPIKEFILTER_H_
#define SPIKEFILTER_H_


class SpikeFilter: public WindowedFilter {
public:
	SpikeFilter(double skp_thresh = 0.5);
	~SpikeFilter();
	void setSpikeThreshold(double d);
	virtual void filter_window(TimeSeries * input, TimeSeries * output, int start, int end);

	double spike_thresh;
};

#endif /* SPIKEFILTER_H_ */
