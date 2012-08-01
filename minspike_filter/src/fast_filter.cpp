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
		log_prog(2,2,"Spike Filter","%d/%d iterations",iter+1,sizeof(windowSizes)/sizeof(double));
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

	log_prog(1,2,"Min Filter","");
	ts = MIN_FILTER.filter(ts);
	ts = SPK_FILTER.filter_iterate(ts, &spk_filter_configure);
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
