/*
 * CSVLoader.h
 *
 *  Created on: Jun 29, 2012
 *      Author: kweekly
 */

#ifndef CSVLOADER_H_
#define CSVLOADER_H_

class CSVLoader {
public:
	static TimeSeries * loadTSfromCSV(string fname);
	static TimeSeries * loadTSfromCSV(string fname,int column);
	static vector<TimeSeries *> loadMultiTSfromCSV(string fname);

	static void writeTStoCSV(string fname, TimeSeries * ts, int precision = 32);
	static void writeMultiTStoCSV(string fname, vector<TimeSeries * > ts, int precision = 32);

};

#endif /* CSVLOADER_H_ */
