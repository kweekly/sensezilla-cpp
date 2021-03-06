/*
 * TimeSeries.cpp
 *
 *  Created on: Jun 29, 2012
 *      Author: kweekly
 */

#include "all.h"


TimeSeries::TimeSeries() {
	metadata.clear();
	t.clear();
	v.clear();
}

TimeSeries::TimeSeries(vector<double> time,vector<double> values){
	t = time;
	v = values;
	metadata.clear();
}

TimeSeries::TimeSeries(vector<double> time,double value) {
	t = time;
	v.clear();
	v.reserve(t.size());
	for ( size_t c = 0; c < t.size(); c++ ) v.push_back(value);
	metadata.clear();
}

TimeSeries::TimeSeries(double tstart, double tend, double interval, double value) {
	t.clear(); t.reserve((int)((tend-tstart)/interval+0.9999));
	v.clear(); v.reserve((int)((tend-tstart)/interval+0.9999));
	for ( double d = tstart; d < tend; d += interval) {
		t.push_back(d);
		v.push_back(value);
	}
	metadata.clear();
}

void TimeSeries::insertPointAtEnd(double time, double value) {
	t.push_back(time);
	v.push_back(value);
}


double TimeSeries::getPoint(double time) {
	return v[findPoint(time)];
}

int TimeSeries::findPoint(double time) {
	// do a binary search
	int iA = 0, iB = t.size();
	while ( iA < iB ) {
		int iC = (iA+iB) / 2;
		if ( t[iC] < time ) {
			iA = iC + 1;
		} else {
			iB = iC;
		}
	}
	return iA;
}

TimeSeries * TimeSeries::selectTime(double t1, double t2) {
	TimeSeries * nts = new TimeSeries();
	int iA = findPoint(t1);
	for ( size_t c = iA; c < t.size() && t[c] <= t2; c++) {
		nts->insertPointAtEnd(t[c],v[c]);
	}
	return nts;
}

TimeSeries * TimeSeries::copy() {
	TimeSeries * ret = new TimeSeries();
	ret->metadata.reserve(metadata.size());
	ret->metadata.assign(metadata.begin(),metadata.end());
	ret->t.reserve(t.size());
	ret->t.assign(t.begin(),t.end());
	ret->v.reserve(v.size());
	ret->v.assign(v.begin(),v.end());
	return ret;
}

TimeSeries * TimeSeries::decimate(int interval) {
	TimeSeries * ret = new TimeSeries();
	ret->metadata.assign(metadata.begin(),metadata.end());
	for ( size_t i = 0; i < t.size(); i += interval ) {
		ret->insertPointAtEnd(t[i],v[i]);
	}
	return ret;
}

TimeSeries * TimeSeries::resample(double tstart, double tend, double tinterval) {
	vector<double> timereq;
	for ( double time = tstart; time <= tend; time += tinterval) {
		timereq.push_back(time);
	}
	return interp(timereq);
}

TimeSeries * TimeSeries::interp(vector<double> times) {
	TimeSeries * ret = new TimeSeries();
	ret->metadata.assign(metadata.begin(),metadata.end());
	ret->t.assign(times.begin(),times.end());
	size_t idx = 0;
	for ( size_t tidx = 0; tidx < times.size(); tidx++) {
		double time = times[tidx];
		while ( idx <= t.size()-1 && time > t[idx] ) idx++;
		double val;
		if ( idx == 0 ) {// uh-oh, before time, use first point
			val = v[0];
		} else if ( idx > t.size() - 1 ) { // uh-oh, after time, use last point
			val = v.back();
		} else {
			val = v[idx-1] + (time-t[idx-1])*(v[idx]-v[idx-1])/(t[idx]-t[idx-1]);
		}
		ret->v.push_back(val);
	}

	return ret;
}

TimeSeries * TimeSeries::interpIntoTime(TimeSeries &other) {
	int tidxL, tidxH;
	if ( other.t[0] <= t[0]) {
		tidxL = 0;
	} else {
		tidxL = findPoint(other.t[0]);
	}

	if ( other.t[other.t.size()-1] >= t[t.size()-1]) {
		tidxH = t.size()-1;
	} else {
		tidxH = findPoint(other.t[other.t.size()-1]) + 1;
	}

	vector<double> timereq;
	for ( int c = tidxL; c <= tidxH; c++ ) {
		timereq.push_back(t[c]);
	}

	return other.interp(timereq);
}

TimeSeries * TimeSeries::resample(double tinterval) {
	return resample(t[0],t[t.size()-1],tinterval);
}

void TimeSeries::timescale(double ratio) {
	for ( size_t c = 0; c < t.size(); c++ ) {
		t[c] *= ratio;
	}
}

void TimeSeries::timeoffset(double time) {
	double t0 = t[0];
	for ( size_t c = 0; c < t.size(); c++ ) {
		t[c] += time - t0;
	}
}

TimeSeries & TimeSeries::operator+=(TimeSeries &other) {
	const TimeSeries * tmp = interpIntoTime(other);
	for ( size_t c = 0; c < v.size(); c++ )
		v[c] += tmp->v[c];
	delete tmp;
	return *this;
}

TimeSeries & TimeSeries::operator*=(TimeSeries &other) {
	const TimeSeries * tmp = interpIntoTime(other);
	for ( size_t c = 0; c < v.size(); c++ )
		v[c] *= tmp->v[c];
	delete tmp;
	return *this;
}
TimeSeries & TimeSeries::operator/=(TimeSeries &other) {
	const TimeSeries * tmp = interpIntoTime(other);
	for ( size_t c = 0; c < v.size(); c++ )
		v[c] /= tmp->v[c];
	delete tmp;
	return *this;
}
TimeSeries & TimeSeries::operator-=(TimeSeries &other) {
	const TimeSeries * tmp = interpIntoTime(other);
	for ( size_t c = 0; c < v.size(); c++ )
		v[c] -= tmp->v[c];
	delete tmp;
	return *this;
}

TimeSeries & TimeSeries::operator+=(const double val) {
	for ( size_t c = 0; c < v.size(); c++ ) {
		v[c] += val;
	}
	return *this;
}
TimeSeries & TimeSeries::operator*=(const double val) {
	for ( size_t c = 0; c < v.size(); c++ ) {
		v[c] *= val;
	}
	return *this;
}
TimeSeries & TimeSeries::operator/=(const double val) {
	for ( size_t c = 0; c < v.size(); c++ ) {
		v[c] /= val;
	}
	return *this;
}
TimeSeries & TimeSeries::operator-=(const double val) {
	for ( size_t c = 0; c < v.size(); c++ ) {
		v[c] -= val;
	}
	return *this;
}

int TimeSeries::find_max() {
	int i = 0;
	for ( size_t c = 1; c < t.size(); c++ )
		if ( v[c] > v[i] ) i = c;
	return i;
}
int TimeSeries::find_min() {
	int i = 0;
	for ( size_t c = 1; c < t.size(); c++ )
		if ( v[c] < v[i] ) i = c;
	return i;
}

double TimeSeries::max() {
	return v[find_max()];
}
double TimeSeries::min() {
	return v[find_min()];
}

double TimeSeries::mean() {
	return sum() / t.size();
}

double TimeSeries::sum() {
	double sumv = 0;
	for ( size_t c = 0; c < t.size(); c++) {
		sumv += v[c];
	}
	return sumv;
}


TimeSeries::~TimeSeries() {
	metadata.clear();
	t.clear();
	v.clear();
}
