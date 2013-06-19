#include "PF_IPS.h"

double tsSTD(TimeSeries *ts);

double tsSTD(TimeSeries *ts) {
	double ssq = 0;
	double mean = ts->mean();
	for (size_t c = 0; c < ts->v.size(); c++ ) {
		ssq += pow(ts->v[c]-mean,2);
	}
	return ssq / ts->v.size();
}

void PF_IPS::transition_model(State & state, Params & params) {

}

double PF_IPS::observation_model(const State & in_state, const Observation & measurement, Params & params) {
	return 1.0;
}