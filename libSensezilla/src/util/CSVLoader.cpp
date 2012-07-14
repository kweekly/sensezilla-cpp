/*
 * CSVLoader.cpp
 *
 *  Created on: Jun 29, 2012
 *      Author: kweekly
 */

#include "all.h"


TimeSeries * CSVLoader::loadTSfromCSV(string fname) {
	return loadTSfromCSV(fname,0);
}

TimeSeries * CSVLoader::loadTSfromCSV(string fname,int column) {
	return loadMultiTSfromCSV(fname)[column];
}

vector<TimeSeries *> CSVLoader::loadMultiTSfromCSV(string fname) {
	ifstream fin(fname.c_str());
	string line;
	if ( !fin.is_open() ) {
		log_e("Cannot open file %s for reading: %s",fname.c_str(), strerror(errno));
		exit(2);
	}
	log_i("Reading CSV File: %s",fname.c_str());

	vector<TimeSeries *> outv;
	outv.push_back(new TimeSeries());
	while(fin.good()) {
		getline(fin, line);
		int first = line.find_first_not_of(" \t\f\v\n\r");
		int last = line.find_last_not_of(" \t\f\v\n\r,");
		if ( first == last ) {
			continue;
		}
		line = line.substr(first,last - first + 1);
		if ( line[0] == '#' ) {
			//log("%s",line.c_str());
			outv[0]->metadata.push_back(line.substr(1));
		} else {
			line = line.append(",");
			int lastpos = 0, pos = 0, idx = 0;
			double time = atof(line.substr(0,lastpos = line.find(',')).c_str());
			pos = line.find(',',lastpos+1);
			while ( pos != string::npos ) {
				double val = atof(line.substr(lastpos+1,pos-lastpos-1).c_str());
				if ( idx >= outv.size()) {
					outv.push_back(new TimeSeries());
					outv[idx]->metadata.assign(outv[0]->metadata.begin(),outv[0]->metadata.end());
				}
				outv[idx]->insertPointAtEnd(time,val);
				idx++;
				lastpos = pos;
				pos = line.find(',',pos+1);
			}
		}
	}
	fin.close();

	log_i("Read %d points in %d columns.",outv[0]->t.size(),outv.size());
	return outv;
}

void CSVLoader::writeTStoCSV(string fname, TimeSeries * ts, int precision){
	vector<TimeSeries *> tmp;
	tmp.push_back(ts);
	writeMultiTStoCSV(fname, tmp, precision);
}
void CSVLoader::writeMultiTStoCSV(string fname, vector<TimeSeries *> ts, int precision) {
	ofstream fout(fname.c_str());
	string line;
	if ( !fout.is_open() ) {
		log_e("Cannot open file %s for writing: %s",fname.c_str(),strerror(errno));
		exit(2);
	}

	fout.precision(precision);

	log("Writing CSV file: %s",fname.c_str());
	for (int c = 0; c < ts[0]->metadata.size(); c++ ) {
		fout << "#" << ts[0]->metadata[c] << endl;
	}

	for ( int c = 0; c < ts[0]->t.size(); c++ ) {
		fout << ts[0]->t[c] << ",";
		for ( int d = 0; d < ts.size(); d++ ) {
			if ( d < ts.size() - 1 ) {
				if ( fabs(ts[d]->v[c]) > 1e-50 )
					fout << 0 << ",";
				else
					fout << ts[d]->v[c] << ",";
			} else {
				if ( fabs(ts[d]->v[c]) > 1e-50 )
					fout << ts[d]->v[c]<<endl;
				else
					fout << 0 <<endl;
			}
		}
	}

	log("Wrote %d points in %d columns.",ts[0]->t.size(),ts.size());



	fout.close();
}
