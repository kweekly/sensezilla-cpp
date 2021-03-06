/*
 * TimeSeries.h
 *
 *  Created on: Jun 29, 2012
 *      Author: kweekly
 */

#ifndef TIMESERIES_H_
#define TIMESERIES_H_

class TimeSeries {

public :
	TimeSeries();
	TimeSeries(vector<double> time,vector<double> values);
	TimeSeries(vector<double> time,double value);
	TimeSeries(double tstart, double tend, double interval, double value);

	void insertPointAtEnd(double time, double value);
	double getPoint(double time);
	int findPoint(double time);
	void timescale(double ratio); // multiply all by ratio
	void timeoffset(double time); // add to all times assuming t[0]==0.0

	TimeSeries * copy();
	TimeSeries * decimate(int interval);
	TimeSeries * resample(double tinterval);
	TimeSeries * resample(double tstart, double tend, double tinterval);
	TimeSeries * interp(vector<double> time);
	TimeSeries * selectTime(double t1, double t2);

	TimeSeries & operator+=(TimeSeries &other);
	TimeSeries & operator*=(TimeSeries &other);
	TimeSeries & operator/=(TimeSeries &other);
	TimeSeries & operator-=(TimeSeries &other);

	TimeSeries & operator+=(const double val);
	TimeSeries & operator*=(const double val);
	TimeSeries & operator/=(const double val);
	TimeSeries & operator-=(const double val);

	int find_max();
	int find_min();
	double max();
	double min();
	double mean();
	double sum();

	vector<string> metadata;
	vector<double> t;
	vector<double> v;

	~TimeSeries();

private:
	TimeSeries * interpIntoTime(TimeSeries &other);
};


#endif /* TIMESERIES_H_ */
