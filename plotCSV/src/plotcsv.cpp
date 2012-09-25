
#include "all.h"
#include <plplot/plstream.h>


int error = 0;


void label_func( int axis, double value, char * text, int length, void * data) {
	if (fabs(value) >= 1e9 ) {
		snprintf(text,length,"%dG",(int)(value/1e9 + 0.5 ));
	}
	else if (fabs(value) >= 1e6) {
		snprintf(text,length,"%dM",(int)(value/1e6 + 0.5 ));
	}
	else if (fabs(value) >= 1e3) {
		snprintf(text,length,"%dk",(int)(value/1000 + 0.5 ));
	}
	else if (fabs(value) >= 1) {
		snprintf(text,length,"%d",(int)(value + 0.5 ));
	}
	else if (fabs(value) >= 1e-3) {
		snprintf(text,length,"%dm",(int)(value*1e3 + 0.5 ));
	}
	else if (fabs(value) >= 1e-6) {
		snprintf(text,length,"%du",(int)(value*1e6 + 0.5 ));
	}
	else if (fabs(value) >= 1e-9) {
		snprintf(text,length,"%dn",(int)(value*1e9 + 0.5 ));
	}
}

class CSVPlotter : public AbstractProgram {
public:
	CSVPlotter() {
		w = 1500;
		h = 300;
		csvprovided = 0;
		pngprovided = 0;
		sah = false;
	}
	~CSVPlotter() {}

	virtual void printHelp() {
		log_i(
			"Creates a PNG plot of a CSV file\n"
			"Kevin Weekly\n"
			"\n"
			"\t-csvin <input file> : CSV file to load from\n"
			"\t-pngout <output file> : PNG file to write to\n"
			"\t-width <width> : Width of image in pixels\n"
			"\t-height <height> : Height of image in pixels\n"
			"\t-xlabel <string> : Label of time axis\n"
			"\t-ylabel <string> : Label for value axis\n"
			"\t-sah : Sample-and-hold vs. linear interpolation\n"
			"\t-title <string> : Title of plot\n"
		);

		AbstractProgram::printHelp();
	}
	virtual void start() {
		if ( !csvprovided ) {
			log_e("No CSV provided.");
			printHelp();
			error = 1;
			return;
		}
		if ( !pngprovided ) {
			log_e("No output png provided.");
			printHelp();
			error = 1;
			return;
		}
		TimeSeries * ts = CSVLoader::loadTSfromCSV(csvin);
		if (ts->t[0] > 1340000000000) {
			log_i("Detected ms timescale.");
			for (int c = 0; c < ts->t.size(); c++ ) {
				ts->t[c] /= 1000;
			}
		}
		vmax = ts->max(), vmin = ts->min();
		tmin = ts->t[0], tmax = ts->t.back();

		if ( sah ) {
			npoints = ts->t.size() * 2 - 2;
			t = new PLFLT[npoints];
			v = new PLFLT[npoints];
			for ( int c = 0; c < ts->t.size() - 1; c++ ) {
				t[c * 2] = ts->t[c];
				v[c * 2] = ts->v[c];
				t[c * 2 + 1] = ts->t[c+1];
				v[c * 2 + 1] = ts->v[c];
			}
		} else {
			t = &(ts->t[0]);
			v = &(ts->v[0]);
			npoints = ts->t.size();
		}


		mkplot();
	}

	void mkplot() {
		plstream * pls = new plstream();
		pls->sdev("pngcairo");
		pls->sfnam(pngout.c_str());
		char buf[128];
		sprintf(buf,"%dx%d",w,h);
		pls->setopt("-geometry",buf);
		pls->scol0(0,255,255,255);
		pls->init();
		pls->scol0(1,0,0,0);
		pls->col0(1);

		pls->schr(0,0.8);
		pls->smaj(0, 1);
		pls->smin(0, 1);
		pls->timefmt("%m/%d %H:%M");

		pls->adv(0);
		pls->vsta();
		pls->wind(tmin,tmax,vmin,vmax);
		pls->slabelfunc(&label_func,NULL);
		pls->box("bcnstdg", 24*3600 , 24, "bcnstvo", (vmax-vmin)/5,0); // set axis and ticks

		//pls->env(tmin,tmax,vmin,vmax, 0, 42);

		/*
		pls->vsta();
		pls->wind( tmin, tmax, vmin, vmax );
		//
		pls->box("bcnstd", 0 , 0, "bcnstv", 0,0); // set axis and ticks
		*/
		pls->lab(xlabel.c_str(),ylabel.c_str(),title.c_str());

		pls->scol0(1,0,0,255);
		pls->col0(1);
		pls->line(npoints, t, v);
		delete pls;
	}

	bool processCLOption(string opt, string val) {
		if ( opt == "csvin" ) {
			csvin = val;
			csvprovided = 1;
			return true;
		} else if ( opt == "pngout" ) {
			pngout = val;
			pngprovided = 1;
			return true;
		} else if ( opt == "width") {
			w = atoi(val.c_str());
			return true;
		} else if (opt == "height" ){
			h = atoi(val.c_str());
			return true;
		} else if (opt == "xlabel") {
			xlabel = val;
			return true;
		} else if ( opt == "ylabel") {
			ylabel = val;
			return true;
		} else if ( opt == "title") {
			title = val;
			return true;
		} else if ( opt == "sah" ){
			sah = true;
			return true;
		}

		return false;
	}

private:
	int w,h;
	string csvin;
	string pngout;
	bool csvprovided;
	bool pngprovided;
	string xlabel;
	string ylabel;
	string title;
	bool sah;

	// for plotting
	int npoints;
	PLFLT * t;
	PLFLT * v;
	PLFLT tmax,tmin,vmax,vmin;
};

int main(int argc,  char * const * argv) {
	CSVPlotter prog;
	prog.parseCL(argc,argv);
	prog.start();
	return error;
}
