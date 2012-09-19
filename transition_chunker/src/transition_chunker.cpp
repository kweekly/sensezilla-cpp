/*
 * library_builder.cpp
 *
 *  Created on: Jul 9, 2012
 *      Author: nerd256
 */
#include "all.h"

int error = 0;

class TransitionChunker : public AbstractProgram {
public:
	TransitionChunker();
	virtual ~TransitionChunker();

	virtual void printHelp();
	virtual void start();

	bool processCLOption(string opt, string val);

private:
	int outprec;

	string csvin_fname;
	string csvout_fname;

	bool use_csv;
};

TransitionChunker::TransitionChunker() {
	csvin_fname.clear();
	csvout_fname.clear();
	use_csv = false;
}
TransitionChunker::~TransitionChunker() {

}

bool TransitionChunker::processCLOption(string opt, string val) {
	if (opt == "csvin") {
		use_csv = true;
		csvin_fname = val;;
		return true;
	}
	else if ( opt == "outprec" ) {
		outprec = atoi(val.c_str());
		return true;
	} else if ( opt == "csvout") {
		csvout_fname = val;
		return true;
	}

	return AbstractProgram::processCLOption(opt, val);
}

void TransitionChunker::printHelp() {
	log_i(
			"Transition Chunker\n"
			"Kevin Weekly\n"
			"\n"
			"\t-csvin  : Input CSV file\n"
			"\t-csvout : Output CSV file\n"
			"\t-outprec :Output precision\n"
			);
	AbstractProgram::printHelp();
}

TransitionDetector TRANS_DETECTOR;

bool cmp_by_timeseries_length(TimeSeries * ts1, TimeSeries * ts2) {
	return (ts1->t.back() - ts1->t.front()) > (ts2->t.back() - ts2->t.front());
}

void TransitionChunker::start() {
	TimeSeries * inputTS = NULL;
	if (use_csv) {
		inputTS = CSVLoader::loadTSfromCSV(csvin_fname);
	} else {
		log_e("No input source defined. Stop.");
		return;
	}

#define NUM_STEPS 5
	TimeSeries * inputRaw = inputTS->copy();

	TRANS_DETECTOR.setParams(0.1,60);
	log_prog(1,NUM_STEPS,"Detect Transitions","");
	EventSeries<TransitionEvent> * es = TRANS_DETECTOR.detect(inputTS);

	log_i("%d Transitions found\n",es->events.size());
	if ( es->events.size() <= 1 ) {
		log_i("Not enough transitions to do anything useful");
		log_prog(NUM_STEPS,NUM_STEPS,"Early abort","No data");
		error = 1;
		return;
	}

	// STEP Ia : SEPARATE INTO CHUNKS
	log_prog(2,NUM_STEPS,"Separate Time Chunks","");
	vector<TimeSeries *> chunks;
	for ( size_t c = 0; c < es->events.size() - 1; c++) {
		chunks.push_back(inputTS->selectTime(es->events[c].t,es->events[c+1].t));
	}

	// STEP Ib : ORDER CHUNKS BY SIZE
	log_prog(3,NUM_STEPS,"Order by Size","");
	sort(chunks.begin(), chunks.end(), cmp_by_timeseries_length);

	// STEP Ic : CATAGORIZE CHUNKS BY MEANS
	double NEW_BIN_THRESH = 2;
	vector<vector<TimeSeries *> *> chunk_bins;
	vector<double> current_means;

	// start with biggest chunk in first bin
	chunk_bins.push_back(new vector<TimeSeries *>);
	chunk_bins[0]->push_back(chunks[0]);
	current_means.push_back(chunks[0]->mean());

	for ( int c = 1; c < chunks.size(); c++ ) {
		log_prog(4,NUM_STEPS,"Categorize by Means","%.2f%%",(100.0*c/chunks.size()));
		double chmean = chunks[c]->mean();

		// find the "closest" bin
		int minmean = 0;
		for ( int d = 1; d < chunk_bins.size(); d++ ) {
			if ( fabs(chmean - current_means[d]) < fabs(chmean - current_means[minmean]) ) {
				minmean = d;
			}
		}
		// if closest bin is too far off, create a new bin
		if ( current_means[minmean] / chmean > NEW_BIN_THRESH || chmean / current_means[minmean] > NEW_BIN_THRESH ) {
			chunk_bins.push_back(new vector<TimeSeries *>);
			chunk_bins.back()->push_back(chunks[c]);
			current_means.push_back(chmean);
		} else { // add to bin and update mean
			current_means[minmean] = (current_means[minmean] * chunk_bins[minmean]->size() + chmean) / (chunk_bins[minmean]->size() + 1);
			chunk_bins[minmean]->push_back(chunks[c]);
		}
	}

	for ( int c = 0; c < chunk_bins.size(); c++ ) {
		log("Cluster %2d : Mean %10.2f",c,current_means[c]);
		for ( int d = 0; d < chunk_bins[c]->size(); d++) {
			log("\tChunk %3d: t:%10.2f mean:%10.2f",d,chunk_bins[c]->at(d)->t.front() - inputTS->t.front(),chunk_bins[c]->at(d)->mean());
		}
	}

	if ( csvout_fname.size() > 0 ) {
		struct stat statret;
		int ret = stat(csvout_fname.c_str(), &statret);
		if ( ret && errno != ENOENT ) {
			log_e("Error: Couldn't stat %s : %s",csvout_fname.c_str(),strerror(errno));
			error = 1;
			goto cleanup;
		} else if ( ret && errno == ENOENT) { // make the directory ourselves
			ret = mkdir(csvout_fname.c_str(), 0777);
			if ( ret ) {
				log_e("Error: Couldn't create output directory %s : %s",csvout_fname.c_str(),strerror(errno));
				error = 1;
				goto cleanup;
			}
		} else if ( !S_ISDIR(statret.st_mode) ) {
			log_e("Error: File %s exists and is not a directory",csvout_fname.c_str());
			error = 1;
			goto cleanup;
		}
		char buf[1024];

		// write the output
		for(int c = 0; c < chunk_bins.size(); c++) {
			log_prog(5,NUM_STEPS,"Write out chunks","%.2f%%",100.0*c/chunk_bins.size());
			for ( int d = 0; d < chunk_bins[c]->size(); d++ ) {
				sprintf(buf, "%s/%d.%d.csv", csvout_fname.c_str(), c, d);
				TimeSeries *tmp = inputRaw->selectTime(chunk_bins[c]->at(d)->t.front(), chunk_bins[c]->at(d)->t.back());
				CSVLoader::writeTStoCSV(string(buf),tmp,outprec);
				delete tmp;
			}
		}

	}
	cleanup:
	// cleanup
	while(!chunk_bins.empty()) delete chunk_bins.back(), chunk_bins.pop_back();
	while(!chunks.empty()) delete chunks.back(), chunks.pop_back();
	delete es;
	delete inputTS;
	delete inputRaw;

	log_i("Done.");
}



int main(int argc,  char * const * argv) {
	TransitionChunker prog;
	prog.parseCL(argc,argv);
	prog.start();
	return error;
}
