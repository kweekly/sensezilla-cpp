//============================================================================
// Name        : fast_filter.cpp
// Author      : Kevin Weekly
// Version     :
// Copyright   : 
// Description : Hello World in C++, Ansi-style
//============================================================================
#include "all.h"

MinFilter MIN_FILTER;
SpikeFilter SPK_FILTER;
const double windowSizes[] = {2,4,6,8,10,12,14,16,18,20,22,24,26,28,30};
const double windowRatio = 10;

DerivativeFilter DERIV_FILTER;

bool spk_filter_configure(AbstractFilter * f, int iter) {
	if ( iter < sizeof(windowSizes)/sizeof(double) ) {
		((SpikeFilter *)f)->setWindowSize(windowSizes[iter] * windowRatio);
		return true;
	}
	return false;
}

const char * HELP_TEXT =
		"Min + Peak Filter\n"
		"Kevin Weekly\n\n"
		"Filter Specific Options\n"
		"\n";

class MinPeakFilter : public AbstractFilterProgram {
public:
	TimeSeries * filter(TimeSeries *);

	virtual void printHelp();
};

TimeSeries * MinPeakFilter::filter(TimeSeries * ts) {
	MIN_FILTER.setWindowSize(60);
	SPK_FILTER.setWindowInterval(1);
	ts = MIN_FILTER.filter(ts);
	ts = SPK_FILTER.filter_iterate(ts, &spk_filter_configure);
//	ts = DERIV_FILTER.filter(ts);

	/*
	TransitionDetector tdet;
	EventSeries<TransitionEvent> * eser = tdet.detect(ts);
	printf("Transition Events\n");
	for ( int c = 0; c < eser->events.size(); c++) {
		printf("\t%10.2f",eser->events[c].t);
		if ( eser->events[c].type == TransitionEvent::RISING) {
			printf(" RISING\n");
		} else {
			printf(" FALLING\n");
		}
	}
	delete eser;
	 */
	return ts;
}

void MinPeakFilter::printHelp() {
	log_i("%s",HELP_TEXT);
	AbstractFilterProgram::printHelp();
}

int main(int argc,  char * const * argv) {
	MinPeakFilter prog;
	prog.parseCL(argc,argv);
	prog.start();
}
