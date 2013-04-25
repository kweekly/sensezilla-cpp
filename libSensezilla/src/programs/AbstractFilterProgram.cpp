/*
 * AbstractFilter.cpp
 *
 *  Created on: Jul 3, 2012
 *      Author: nerd256
 */

#include "all.h"

AbstractFilterProgram::AbstractFilterProgram() {
	use_csv = false;
	inputTS = NULL;
	in_time_ratio = 1.0;
	out_time_ratio = 1.0;
	in_value_ratio = 1.0;
	out_value_ratio = 1.0;
	out_precision = 32;
}
AbstractFilterProgram::~AbstractFilterProgram() {
}

void AbstractFilterProgram::printHelp() {
	log_i("Filter Options\n"
		"\t-csvin  : Input CSV file\n"
		"\t-intr   : Input time ratio (divide all times by this number)\n"
		"\t-invr   : Input value ratio (...)\n"
		"\t-csvout : Output CSV file\n"
		"\t-outtr  : Output time ratio (mult. all times by this number)\n"
		"\t-outvr  : Output value ratio (...)\n"
	    "\t-outprec: Output precision (for ASCII)\n"
	);
	AbstractProgram::printHelp();
}

TimeSeries * AbstractFilterProgram::filter(TimeSeries * ts) {
	return ts;
}

bool AbstractFilterProgram::processCLOption(string opt, string val) {
	if (opt == "csvin") {
		use_csv = true;
		csvin_fname = val;;
		return true;
	} else if (opt == "csvout") {
		csvout_fname = val;
		return true;
	} else if ( opt == "intr") {
		in_time_ratio = atof(val.c_str());
		return true;
	} else if ( opt == "outtr") {
		out_time_ratio = atof(val.c_str());
		return true;
	} else if ( opt == "invr") {
		in_value_ratio = atof(val.c_str());
		return true;
	} else if ( opt == "outvr") {
		out_value_ratio = atof(val.c_str());
		return true;
	} else if ( opt == "outprec") {
		out_precision = atoi(val.c_str());
		return true;
	}

	return AbstractProgram::processCLOption(opt, val);
}

void AbstractFilterProgram::start() {
	if (use_csv) {
		inputTS = CSVLoader::loadTSfromCSV(csvin_fname);
	} else {
		log_e("No input source defined. Stop.");
		printHelp();
		return;
	}
	for ( size_t c = 0; c < inputTS->t.size(); c++ )
		inputTS->t[c] /= in_time_ratio;

	*inputTS /= in_value_ratio;

	TimeSeries * outputTS = filter(inputTS);

	*outputTS *= out_value_ratio;
	for ( size_t c = 0; c < outputTS->t.size(); c++ )
			outputTS->t[c] *= out_time_ratio;

	if ( !csvout_fname.empty() ){
		printf("%p %.2f %.2f\n",outputTS,outputTS->t[4625] - outputTS->t[0],outputTS->v[4625]);
		CSVLoader::writeTStoCSV(csvout_fname, outputTS, out_precision);
	}
	inputTS = NULL;
	delete outputTS;
}

