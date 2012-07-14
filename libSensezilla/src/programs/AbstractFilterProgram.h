/*
 * AbstractFilter.h
 *
 *  Created on: Jul 3, 2012
 *      Author: nerd256
 */

#ifndef ABSTRACTFILTER_PROGRAM_H_
#define ABSTRACTFILTER_PROGRAM_H_


class AbstractFilterProgram: public AbstractProgram {
public:
	AbstractFilterProgram();
	virtual ~AbstractFilterProgram();

	virtual bool processCLOption( string opt, string val );
	virtual void start();
	virtual void printHelp();
	virtual TimeSeries * filter(TimeSeries * ts);

private:
	TimeSeries * inputTS;

	string csvin_fname;
	string csvout_fname;

	double in_time_ratio;
	double out_time_ratio;
	double in_value_ratio;
	double out_value_ratio;

	int out_precision;

	bool use_csv;
};

#endif /* ABSTRACTFILTER_H_ */
