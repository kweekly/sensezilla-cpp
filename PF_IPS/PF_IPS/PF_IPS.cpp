// PF_IPS.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "all.h"
#include "ShortestPathMap.h"
#include "Grid.h"
#include "PF_IPS.h"
#include "SIRFilter.h"
#include "Visualization.h"


#include <thread>
#include <random>

int error = 0;


PF_IPS::PF_IPS() {
	disp_help = use_rssi = use_xy = use_trajout = use_partout = use_mappng = mappng_bounds_provided = moverwrite = false;
	cellwidth = 1;
	movespeed = 1.25;
	//movespeed = 0.6;
	nParticles = 1000;
	time_interval = 5.0;
	pathmap = NULL;
	req_data_lock = false;
	simulate = false;
	TIME = -1;
}
PF_IPS::~PF_IPS() {
}

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
	} else if ( opt == "cellwidth") {
		try{
			cellwidth = std::stod(val);
		} catch(exception e) {
			log_e("Error in parsing -cellwidth");
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
	}  else if ( opt == "dt" ) {
		try{
			time_interval = std::stod(val);
		} catch (exception e) {
			log_e("Error parsing dt");
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
	} else if ( opt == "groundtruth") {
		groundtruth_fname = val;
		return true;
	} else if ( opt == "gtorigin" ) {
		const char * gtstr = val.c_str();
		gt_ref_X = atof(gtstr);
		if (!(gtstr = strchr(gtstr,','))) goto mberror;
		gt_ref_Y = atof(++gtstr);
		return true;
gterror:
		error = 1;
		log_e("Error in gtorigin string");
		return true;
	} else if ( opt == "simulate") {
		simulate = true;
		return true;
	}


	return AbstractProgram::processCLOption(opt,val);
}

void PF_IPS::_loadRSSIData() {
		rssi_config = ConfigurationLoader::readConfiguration(rssiparam_fname);

		rssi_basedir.assign("./");
		size_t pos1 = rssiparam_fname.rfind("\\");
		size_t pos2 = rssiparam_fname.rfind("/");
		if ( (pos1 < pos2 || pos1 == string::npos) && pos2 != string::npos) {
			rssi_basedir = rssiparam_fname.substr( 0, pos2 + 1 );
		} else if ( (pos2 < pos1 || pos2 == string::npos) && pos1 != string::npos ) {
			rssi_basedir = rssiparam_fname.substr( 0, pos1 + 1 );
		} 

		sense_ref_X = rssi_config[string("sensors")][string("refX")].asDouble();
		sense_ref_Y = rssi_config[string("sensors")][string("refY")].asDouble();
		sense_ref_Z = rssi_config[string("sensors")][string("refZ")].asDouble();

		vector<vector<TimeSeries *>> allrssi;
		int sidx = 0;
		for ( vector<ConfigurationValue>::iterator iter = rssi_config[string("sensors")][string("active_list")].begin(); iter != rssi_config[string("sensors")][string("active_list")].end(); iter++) {
			string id = (*iter).asString();
			RSSISensor sense;
			sense.IDstr = id;
			string posstr = rssi_config[string("sensors")][string("pos_")+id].asString();
			sense.pos.y = std::stof(posstr.substr(0,posstr.find(' ')))  + sense_ref_Y;
			sense.pos.x = std::stof(posstr.substr(posstr.rfind(' ')+1)) + sense_ref_X;
			log_i("RFID sensor %s at (%.2f, %.2f)",sense.IDstr.c_str(),sense.pos.x,sense.pos.y);
			
			vector<TimeSeries *> gaussts = CSVLoader::loadMultiTSfromCSV(rssi_basedir+string("gaussparams_s")+id+string(".csv"));
			sense.gauss_calib_r = gaussts[0]->t;
			sense.gauss_calib_mu = gaussts[0]->v;
			sense.gauss_calib_sigma = gaussts[1]->v;
			delete gaussts[0];
			delete gaussts[1];

			sensors.push_back( sense );
			allrssi.push_back(vector<TimeSeries *>());
			for ( vector<ConfigurationValue>::iterator tag_iter = rssi_config[string("tags")][string("active_list")].begin(); tag_iter != rssi_config[string("tags")][string("active_list")].end(); tag_iter++) {
				string tid = (*tag_iter).asString();
				TimeSeries * rssits = CSVLoader::loadTSfromCSV(rssi_basedir+string("RSSI_t")+tid+string("_s")+id+string(".csv"));
				if ( rssits ) rssits->timescale(1e-3);
				allrssi[sidx].push_back(rssits);
			}
			sidx++;
		}

		// finding min and max times
		double mint=9e99, maxt=-9e99;
		for ( int i = 0; i < allrssi.size(); i++) {
			for ( int j = 0; j < allrssi[i].size(); j++) {
				if ( allrssi[i][j] == NULL || allrssi[i][j]->t.size() == 0 ) continue;
				if ( allrssi[i][j]->t[0] < mint ) {
					mint = allrssi[i][j]->t[0];
				}
				if ( allrssi[i][j]->t.back() > maxt ) {
					maxt = allrssi[i][j]->t.back();
				}
			}
		}
		vector<double> T;
		T.reserve((int)((maxt-mint)/time_interval) + 1);
		for ( double t = mint; t <= maxt; t += time_interval) {
			T.push_back(t);
		}
		log_i("Have sensor data from t=%.0f to t=%.0f for a total of %d points (dt=%.2f)",mint,maxt,T.size(),time_interval);
		TimeSeries * gtx,*gty;
		if ( gtdata.size() > 0 ) {
			log_i("Interpolating ground truth.");
			*(gtdata[0]) += gt_ref_X;
			*(gtdata[1]) += gt_ref_Y;

			log_i("\t(Note: Referenced to RSSI system, and x-y coordinates flipped)");
			gtx = gtdata[1];
			gtdata[1] = gtdata[0];
			gtdata[0] = gtx;

			gtdata[0]->timeoffset( mint );
			gtdata[1]->timeoffset( mint );
			*(gtdata[1]) *= -1;

			gtx = gtdata[0]->interp(T);
			gty = gtdata[1]->interp(T);

			delete gtdata[0];
			delete gtdata[1];

			gtdata[0] = gtx;
			gtdata[1] = gty;
		}

		default_random_engine rengine;
		normal_distribution<double> stdnorm(0.0,1.0);

		for ( int i = 0; i < allrssi.size(); i++ ) {
			if ( simulate ) {
				log_i("Simulating data for sensor %d",i);
				
				TimeSeries * ts = gtx->copy();
				for ( size_t ti = 0; ti < T.size(); ti++) {
					xycoords gtpos = {gtx->v[ti],gty->v[ti]};
					xycoords spos = sensors[i].pos;
					double d = dist(gtpos,spos);
					double mu,sigma;
					_get_gaussian_parameters(&(sensors[i]),d,mu,sigma);
					ts->v[ti] = stdnorm(rengine) * sigma + mu;
				}
				rssi_data.push_back(ts);
			} else {
				log_i("Interpolating data for sensor %d",i);
			
				int n = 0;
				bool init = false;
				for ( int j = 0; j < allrssi[i].size(); j++ ) {
					if ( allrssi[i][j] == NULL || allrssi[i][j]->t.size() < 5 ) continue;
					if ( !init ) {
						rssi_data.push_back(allrssi[i][j]->interp(T));
						init = true;
					} else {
						*(rssi_data.back()) += *(allrssi[i][j]->interp(T));
					}
					n++;
				}

				*(rssi_data.back()) /= n;
			}
		}

error:
		//cleanup
		while (allrssi.size() > 0){
			while(allrssi.back().size() > 0) {
				delete allrssi.back().back();
				allrssi.back().pop_back();
			}
			allrssi.pop_back();
		}

}

void PF_IPS::_calibrate_stat_distributions() {


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
		grid = Grid(bounds,cellwidth,2);
		int movespeed_hexes = (int)(movespeed * time_interval / cellwidth + 0.9999);
		log_i("Move speed=%.2f m/s ( %d hexes / timestep )",movespeed,movespeed_hexes);

		bool load_from_PNG = true;

		if ( mapcache_fname != "" ) { // see if cache is available
			pathmap = ShortestPathMap::loadFromCache(mapcache_fname, mappng_fname, grid, movespeed_hexes);
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
		string Grid_str = grid.toString();
		log_i("Grid: %s",Grid_str.c_str());

		if ( load_from_PNG ) {
			PNGData data = PNGLoader::loadFromPNG(mappng_fname);
			log_i("Loading map from %s",mappng_fname.c_str());
			
			log_i("Reading obstacle part");
			vector<vector<double>> obsmap(data.height);
			for ( int row = 0; row < data.height; row++ ) {
				obsmap[row].resize(data.width);
				for ( int col = 0; col < data.width; col++) {
					obsmap[row][col] = (data.B[row][col] == data.R[row][col] && data.R[row][col] == data.G[row][col] && data.R[row][col] < 0.95);
				}
			}
			log_i("Freeing PNG data");
			PNGLoader::freePNGData(&data);

			log_i("Interpolating into hexgrid");
			obsmap = grid.interpolateXYData(obsmap,bounds);
			
			log_i("Thresholding");
			vector<vector<bool>> obsmap_th(obsmap.size());
			for ( size_t j = 0; j < obsmap.size(); j++ ) {
				obsmap_th[j].resize(obsmap[j].size());
				for ( size_t i = 0; i < obsmap[j].size(); i++ ) {
					obsmap_th[j][i] = obsmap[j][i] > 0.2;
				}
			}

			log_i("Calculating shortest path");
			pathmap = ShortestPathMap::generateFromObstacleMap(obsmap_th, mappng_fname, grid, movespeed_hexes );

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

	if ( !groundtruth_fname.empty() ) {
		gtdata = CSVLoader::loadMultiTSfromCSV(groundtruth_fname);
		if (gtdata.size() < 2) {
			log_e("Could not open ground truth file");
			goto error;
		}
	} else if ( simulate ) {
		log_e("Simulation specified, but no ground truth file");
		goto error;
	}
		

	if ( use_rssi ) {
		_loadRSSIData();
		if (error) goto error;
	}

	if ( use_xy ) {
		_loadXYData();
		if (error) goto error;
	}


	if (statein_fname != "") {

	}

	_loadMapPNG();
	if (error) goto error;

	_sirFilter();

	if (stateout_fname != "") {

	}
	

	error:
	// clean-up
	if ( pathmap ) delete pathmap;
	while(!gtdata.empty()) {
		delete gtdata.back();
		gtdata.pop_back();
	}
	while(!rssi_data.empty()) {
		delete rssi_data.back();
		rssi_data.pop_back();
	}
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
		  "\t-cellwidth : Radius (in m) to discretize space\n"
		  "\t-mapcache  : Map cache (will create if necessary)\n"
		  "\t-moverwrite: Overwrite map cache if parameters are different\n"
		  "\n"
		  "\t-statein   : Static state file\n"
		  "\t-stateout  : Static state file (can be same as statein)\n"
		  "\t-particles : Number of particles\n"
		  "\t-movespeed : Move Speed in m/s\n"
		  "\n"
		  "\t-groundtruth : Ground truth CSV\n"
		  "\t-gtorigin	  : Origin of the ground truth\n"
		  "\t-simulate	  : Generate simulated measurements using groundtruth\n"
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

	Visualization viz(&prog);
	if ( !viz.start() ) {
		log_e("Error: Could not start visualization");
		return -1;
	}

	prog.start();	
	return error;
}

