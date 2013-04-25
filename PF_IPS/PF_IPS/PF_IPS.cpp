// PF_IPS.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "all.h"
#include "ShortestPathMap.h"
#include "HexMap.h"

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
	double movespeed;
	
	int nParticles;

	string rssiparam_fname, xyparam_fname, trajout_fname, partout_fname;
	string mappng_fname, mapcache_fname, statein_fname, stateout_fname;
};

PF_IPS::PF_IPS() {
	disp_help = use_rssi = use_xy = use_trajout = use_partout = use_mappng = mappng_bounds_provided = moverwrite = false;
	hexradius = 0.5;
	movespeed = 1.5;
	nParticles = 1000;
}
PF_IPS::~PF_IPS() {}

bool PF_IPS::processCLOption( string opt, string val ) {
	if ( opt == "help" ) {
		disp_help = true;
		return true;
	} else if ( opt == "rssiparam") {
		use_rssi = true;
		rssiparam_fname = val;
		return true;
	} else if ( opt == "xyparam") {
		use_xy = true;
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
	} else if ( opt == "movespeed") {
		try {
			movespeed = std::stod(val);
		} catch(exception e) {
			log_e("Error in parsing -movespeed");
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

	map<string,map<string, ConfigurationValue>> xy_config;
	
	map<string,map<string, ConfigurationValue>> rssi_config;
	vector<RSSISensor> sensors;
	vector<RSSITag> reference_tags;

	if ( use_rssi ) {
		rssi_config = ConfigurationLoader::readConfiguration(rssiparam_fname);

		for ( vector<ConfigurationValue>::iterator iter = rssi_config[string("sensors")][string("active_list")].begin(); iter != rssi_config[string("sensors")][string("active_list")].end(); iter++) {
			string id = (*iter).asString();
			RSSISensor sense;
			sense.IDstr = id;
			string posstr = rssi_config[string("sensors")][string("pos_")+id].asString();
			sense.pos.x = std::stof(posstr.substr(0,posstr.find(' ')));
			sense.pos.y = std::stof(posstr.substr(posstr.rfind(' ')+1));
			log_i("RFID sensor %s at (%.2f, %.2f)",sense.IDstr.c_str(),sense.pos.x,sense.pos.y);
			sensors.push_back( sense );
		}

		for ( vector<ConfigurationValue>::iterator iter = rssi_config[string("tags")][string("reference_list")].begin(); iter != rssi_config[string("tags")][string("reference_list")].end(); iter++) {
			string id = (*iter).asString();
			RSSITag tag;
			tag.IDstr = id;
			tag.isref = true;
			string posstr = rssi_config[string("tags")][string("pos_")+id].asString();
			tag.pos.x = std::stof(posstr.substr(0,posstr.find(' ')));
			tag.pos.y = std::stof(posstr.substr(posstr.rfind(' ')+1));
			log_i("RFID reference tag %s at (%.2f, %.2f)",tag.IDstr.c_str(),tag.pos.x,tag.pos.y);
			reference_tags.push_back( tag );			
		}
	}
	if ( use_xy ) {
		xy_config = ConfigurationLoader::readConfiguration(xyparam_fname);
	}

	ShortestPathMap * pathmap = NULL;

	if ( use_mappng ) {
		if ( !mappng_bounds_provided ) {
			log_e("Must provide bounds of map PNG file provided");
			error = 1;
			goto error;
		}

		if ( minx >= maxx || miny >= maxy ) {
			log_e("Max-x or max-y bigger than max-x/min-y\nMake sure format is: minx,maxx,miny,maxy");
			error = 1;
			goto error;
		}

		double bounds[4] = { minx, maxx, miny, maxy };
		HexMap hmap(bounds,hexradius,2);
		int movespeed_hexes = (int)(movespeed / hexradius + 0.9999);

		bool load_from_PNG = true;

		if ( mapcache_fname != "" ) { // see if cache is available
			pathmap = ShortestPathMap::loadFromCache(mapcache_fname, mappng_fname, hmap, movespeed_hexes);
			if ( pathmap ) {
				log_i("Map cache loaded, no need to recalculate");
				load_from_PNG = false;
			} else {
				if ( !moverwrite ) {
					log_e("Map cache invalid and moverwrite FALSE, aborting.");
					error = 1;
					goto error;
				}
				log_i("Map cache invalid and will be recalculated.");
			}
		}

		double dimx = maxx - minx, dimy = maxy - miny;
		log_i("Dimensions of map: (%.2f m x %.2f m)  %.2f sq.m.",dimx,dimy,dimx*dimy);
		log_i("Bounds: [%.2f %.2f %.2f %.2f]",minx,maxx,miny,maxy);
		string hexmap_str = hmap.toString();
		log_i("Hex map: %s",hexmap_str.c_str());

		if ( load_from_PNG ) {
			PNGData data = PNGLoader::loadFromPNG(mappng_fname);
			log_i("Loading map from %s",mappng_fname.c_str());
			
			log_i("Reading obstacle part");
			vector<vector<double>> obsmap(data.height);
			for ( int row = 0; row < data.height; row++ ) {
				obsmap[row].resize(data.width);
				for ( int col = 0; col < data.width; col++) {
					obsmap[row][col] = (data.B[row][col] == data.R[row][col] && data.R[row][col] == data.G[row][col] && data.R[row][col] != 255);
				}
			}
			log_i("Freeing PNG data");
			PNGLoader::freePNGData(&data);

			log_i("Interpolating into hexgrid");
			obsmap = hmap.interpolateXYData(obsmap,bounds);
			
			log_i("Thresholding");
			vector<vector<bool>> obsmap_th(obsmap.size());
			for ( size_t j = 0; j < obsmap.size(); j++ ) {
				obsmap_th[j].resize(obsmap[j].size());
				for ( size_t i = 0; i < obsmap[j].size(); i++ ) {
					obsmap_th[j][i] = obsmap[j][i] > 0.2;
				}
			}

			log_i("Calculating shortest path");
			pathmap = ShortestPathMap::generateFromObstacleMap(obsmap_th, mappng_fname, hmap, movespeed_hexes );

			if ( mapcache_fname != "" && moverwrite  ){
				log_i("Writing mapcache");
				pathmap->saveToCache(mapcache_fname);
			}
		}

	} else {
		if ( mapcache_fname == "" ) {
			log_i("Warning: Map undefined ( no cache or png supplied )");	
		} else { // load everything from mapcache
			pathmap = ShortestPathMap::loadFromCache(mapcache_fname);
			if (!pathmap) {
				error = 1;
				log_e("Error: Couldn't load pathmap");
				goto error;
			}
		}
	}

	if (statein_fname != "") {

	}


	// do the processing

	if (stateout_fname != "") {

	}

	error:
	// clean-up
	;
}

void PF_IPS::printHelp() {
	log_i("Particle Filter Indoor Positioning System\n"
	      "Kevin Weekly\n"
		  "-----------------------------------------\n"
		  "PF Options\n"
		  "\t-rssiparam : RSSI input parameters\n"
		  "\n"
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
		  "\t-movespeed : Move Speed in m/s\n"
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

