/*
 * TransitionDetector.cpp
 *
 *  Created on: Jul 9, 2012
 *      Author: nerd256
 */

#include "all.h"

TransitionDetector::TransitionDetector(double th, double d) {
	setParams(th,d);
}

TransitionDetector::~TransitionDetector() {
}

void TransitionDetector::setParams(double th, double de) {
	thresh = th;
	deci = de;
}

EventSeries<TransitionEvent> * TransitionDetector::detect(TimeSeries * ts) {
	DerivativeFilter dfilt(deci);
	ts = dfilt.filter(ts->copy()); // we agree not to kill the input argument

	// Threshold is a ratio of max to min value
	double tot_range = ts->max() - ts->min();
	double thresh_scaled = thresh * (tot_range);
	log_i("Threshold: %.2f (%.2f%% of %.2f)",thresh_scaled,thresh*100,tot_range);

	bool found_spike = false;
	EventSeries<TransitionEvent>  * eser = new EventSeries<TransitionEvent>();
	for ( size_t c= 1; c < ts->t.size() - 1; c++ ) {
		// not only over threshold, but find the peak
		if ( ts->v[c] >= thresh_scaled && ts->v[c-1] <= ts->v[c] && ts->v[c+1] <= ts->v[c]) {
			if ( !found_spike )
				eser->addEventAtEnd(TransitionEvent(ts->t[c],TransitionEvent::RISING));
			found_spike = true;
		} else if ( ts->v[c] <= -thresh_scaled && ts->v[c-1] >= ts->v[c] && ts->v[c+1] >= ts->v[c] ) {
			if ( !found_spike )
				eser->addEventAtEnd(TransitionEvent(ts->t[c],TransitionEvent::FALLING));
			found_spike = true;
		} else {
			found_spike = false;
		}
	}

	delete ts;
	return eser;
}
