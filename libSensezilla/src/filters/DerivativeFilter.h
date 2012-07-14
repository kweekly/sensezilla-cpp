/*
 * DerivativeFilter.h
 *
 *  Created on: Jul 11, 2012
 *      Author: nerd256
 */

#ifndef DERIVATIVEFILTER_H_
#define DERIVATIVEFILTER_H_

class DerivativeFilter : public AbstractFilter {
public:
	DerivativeFilter(double winsize = 60);
	void setWindowSize(double w);
	virtual ~DerivativeFilter();

	virtual void filter_into(TimeSeries * in, TimeSeries * out);

private:
	double window_size;
};

#endif /* DERIVATIVEFILTER_H_ */
