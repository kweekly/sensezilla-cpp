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

	void insertPointAtEnd(double time, double value);
	double getPoint(double time);
	int findPoint(double time);

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

	vector<string> metadata;
	vector<double> t;
	vector<double> v;

	~TimeSeries();

private:
	TimeSeries * interpIntoTime(TimeSeries &other);
};


#endif /* TIMESERIES_H_ */
