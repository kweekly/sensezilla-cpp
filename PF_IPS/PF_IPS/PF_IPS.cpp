// PF_IPS.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "all.h"

#include <thread>

int error = 0;

class PF_IPS : public AbstractProgram {
public:
	PF_IPS();
	~PF_IPS();
	
	bool processCLOption( string opt, string val );
	void start();
	void printHelp();

private:
	bool disp_help;

	bool use_rssi;
	bool use_xy;
	bool use_trajout;
	bool use_partout;
	
	bool use_mappng;
	bool mappng_bounds_provided;
	bool moverwrite;

	double minx,miny,maxx,maxy;
	double hexradius;
	
	int nParticles;

	string rssi_fname, rssiparam_fname, xyloc_fname, xyparam_fname, trajout_fname, partout_fname;
	string mappng_fname, mapcache_fname, statein_fname, stateout_fname;
};

PF_IPS::PF_IPS() {
	disp_help = use_rssi = use_xy = use_trajout = use_partout = use_mappng = mappng_bounds_provided = moverwrite = false;
	hexradius = 5;
	nParticles = 1000;
}
PF_IPS::~PF_IPS() {}

bool PF_IPS::processCLOption( string opt, string val ) {
	if ( opt == "help" ) {
		disp_help = true;
		return true;
	} else if ( opt == "rssiin") {
		use_rssi = true;
		rssi_fname = val;
		return true;
	} else if ( opt == "rssiparam") {
		rssiparam_fname = val;
		return true;
	} else if ( opt == "xyloc") {
		xyloc_fname = val;
		use_xy = true;
		return true;
	} else if ( opt == "xyparam") {
		xyparam_fname = val;
		return true;
	} else if ( opt == "mappng") {
		mappng_fname = val;
		use_mappng = true;
		return true;	
	} else if (opt == "mapbounds") {
		const char * mbstr = val.c_str();
		minx = atof(mbstr);
		if (!(mbstr = strchr(mbstr,','))) goto mberror;
		maxx = atof(++mbstr);
		if (!(mbstr = strchr(mbstr,','))) goto mberror;
		miny = atof(++mbstr);
		if (!(mbstr = strchr(mbstr,','))) goto mberror;
		maxy = atof(++mbstr);
		mappng_bounds_provided = true;
		return true;
mberror:
		error = 1;
		log_e("Error in mapbounds string");
		return true;
	} else if ( opt == "hexradius") {
		try{
			hexradius = std::stod(val);
		} catch(exception e) {
			log_e("Error in parsing -hexradius");
			error = 1;
		}
		return true;
	} else if ( opt == "mapcache") {
		mapcache_fname = val;
		return true;
	} else if ( opt == "moverwrite") {
		moverwrite = true;
		return true;
	} else if ( opt == "statein") {
		statein_fname = val;
		return true;
	} else if ( opt == "stateout") {
		stateout_fname = val;
		return true;	
	} else if ( opt == "particles") {
		try {
			nParticles = std::stoi(val);
		} catch(exception e) {
			log_e("Error in parsing -particles");
		}
		return true;
	} else if ( opt == "trajout") {
		trajout_fname = val;
		use_trajout = true;
		return true;
	} else if ( opt == "partout") {
		partout_fname = val;
		use_partout = true;
		return true;
	}


	return AbstractProgram::processCLOption(opt,val);
}

void PF_IPS::start() {
	if ( disp_help ) {
		printHelp();
		return;
	}

	vector<TimeSeries *> rssi_data;
	vector<TimeSeries *> xy_data;

	if ( use_rssi ) {
		if ( rssiparam_fname == "" ) {
			log_e("Error: Need RSSI Parameter argument if using rssi\n");
			error = 1;
			return;
		}
		rssi_data = CSVLoader::loadMultiTSfromCSV(rssi_fname);
	}
	if ( use_xy ) {
		if ( xyparam_fname == "" ) {
			log_e("Error: Need XY Parameter argument if using xy\n");
			error = 1;
			return;
		}
		xy_data = CSVLoader::loadMultiTSfromCSV(xyloc_fname);
	}

	if ( use_mappng ) {
		if ( !mappng_bounds_provided ) {
			log_e("Must provide bounds of map PNG file provided\n");
			error = 1;
			return;
		}

		if ( minx >= maxx || miny >= maxy ) {
			log_e("Max-x or max-y bigger than max-x/min-y\nMake sure format is: minx,maxx,miny,maxy");
			error = 1;
			return;
		}

		if ( mapcache_fname != "" ) { // see if cache is available

		}

		double dimx = maxx - minx, dimy = maxy - miny;
		log_i("Dimensions of map: (%.2f m x %.2f m)  %.2f sq.m.",dimx,dimy,dimx*dimy);

	} else {
		if ( mapcache_fname == "" ) {
			log_i("Warning: Map undefined ( no cache or png supplied )");	
		} else { // load everything from mapcache

		}
	}

	if (statein_fname != "") {

	}


	// do the processing

	if (stateout_fname != "") {

	}
}

void PF_IPS::printHelp() {
	log_i("Particle Filter Indoor Positioning System\n"
	      "Kevin Weekly\n"
		  "-----------------------------------------\n"
		  "PF Options\n"
		  "\t-rssiin    : Input file of RSSI timeseries\n"
		  "\t-rssiparam : RSSI input parameters\n"
		  "\n"
		  "\t-xyloc     : Input file of XY localization system\n"
		  "\t-xyparam   : XY parameters\n"
		  "\n"
		  "\t-mappng    : Color-coded map file of space\n"
		  "\t-mapbounds : Bounds of map (in m)\n"
		  "\t             Format: minx,maxx,miny,maxy\n"
		  "\t-hexradius : Radius (in m) to discretize space\n"
		  "\t-mapcache  : Map cache (will create if necessary)\n"
		  "\t-moverwrite: Overwrite map cache if parameters are different\n"
		  "\n"
		  "\t-statein   : Static state file\n"
		  "\t-stateout  : Static state file (can be same as statein)\n"
		  "\t-particles : Number of particles\n"
		  "\n"
		  "\t-trajout   : Max-likelihood state output file\n"
		  "\t-partout   : Particle state estimate and weights\n"
		  );

	AbstractProgram::printHelp();
}



int _tmain(int argc, _TCHAR* argv[])
{
	PF_IPS prog;
	prog.parseCL( argc, argv );
	if ( error ) {
		return error;
	}
	prog.start();
	return error;
}

