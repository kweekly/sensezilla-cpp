// PF_IPS.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "all.h"
#include "ShortestPathMap.h"
#include "HexMap.h"
#include "PF_IPS.h"

#include <thread>

int error = 0;


PF_IPS::PF_IPS() {
	disp_help = use_rssi = use_xy = use_trajout = use_partout = use_mappng = mappng_bounds_provided = moverwrite = false;
	hexradius = 0.5;
	movespeed = 1.5;
	nParticles = 1000;
	pathmap = NULL;
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

void PF_IPS::_loadRSSIData() {
		rssi_config = ConfigurationLoader::readConfiguration(rssiparam_fname);

		rssi_basedir.assign("./");
		size_t pos1 = rssiparam_fname.rfind("\\");
		size_t pos2 = rssiparam_fname.rfind("/");
		if ( pos1 < pos2 && pos2 != string::npos) {
			rssi_basedir = rssiparam_fname.substr( 0, pos2 + 1 );
		} else if ( pos2 < pos1 && pos1 != string::npos ) {
			rssi_basedir = rssiparam_fname.substr( 0, pos1 + 1 );
		} 

		sense_ref_X = rssi_config[string("sensors")][string("refX")].asDouble();
		sense_ref_Y = rssi_config[string("sensors")][string("refY")].asDouble();
		sense_ref_Z = rssi_config[string("sensors")][string("refZ")].asDouble();

		tag_ref_X = rssi_config[string("tags")][string("refX")].asDouble();
		tag_ref_Y = rssi_config[string("tags")][string("refY")].asDouble();
		tag_ref_Z = rssi_config[string("tags")][string("refZ")].asDouble();

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

		rssi_refdata = vector<vector<TimeSeries *>>(reference_tags.size(),vector<TimeSeries *>(sensors.size(),NULL));
		log_i("Loading reference tag dataset...");
		for ( size_t refi = 0; refi < reference_tags.size(); refi++ ) {
			for ( size_t seni = 0; seni < sensors.size(); seni++ ) {
				string fn = rssi_config[string("rssidata")][reference_tags[refi].IDstr + "_" + sensors[seni].IDstr].asString();
				if ( fn.size() == 0 ) {
					log_e("\tNo data provided for tag:%s sensor:%s",reference_tags[refi].IDstr,sensors[seni].IDstr);
				} else {
					fn = rssi_basedir + fn;
					log_i("\tLoading %s",fn.c_str());
					rssi_refdata[refi][seni] = CSVLoader::loadTSfromCSV(fn);
				}
			}
		}
}

void PF_IPS::_loadXYData() {
	xy_config = ConfigurationLoader::readConfiguration(xyparam_fname);

}

void PF_IPS::_loadMapPNG() {
	if ( use_mappng ) {
		if ( !mappng_bounds_provided ) {
			log_e("Must provide bounds of map PNG file provided");
			error = 1;
			return;
		}

		if ( minx >= maxx || miny >= maxy ) {
			log_e("Max-x or max-y bigger than max-x/min-y\nMake sure format is: minx,maxx,miny,maxy");
			error = 1;
			return;
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
					return;
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
				return;
			}
		}
	}
}

void PF_IPS::start() {
	if ( disp_help ) {
		printHelp();
		return;
	}

	if ( use_rssi ) {
		_loadRSSIData();
		if (error) goto error;
		_calibrate_RSSI_system();
	}

	if ( use_xy ) {
		_loadXYData();
		if (error) goto error;
	}


	if (statein_fname != "") {

	}

	_loadMapPNG();
	if (error) goto error;

	// do the processing



	if (stateout_fname != "") {

	}

	error:
	// clean-up
	while ( rssi_refdata.size() > 0 ) { 
		vector<TimeSeries *> subvec = rssi_refdata.back();
		while ( subvec.size() > 0 ) {
			delete subvec.back();
			subvec.pop_back();
		}
		rssi_refdata.pop_back();
	}
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

