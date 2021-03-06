// PF_IPS.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "all.h"
#include "ShortestPathMap.h"
#include "Grid.h"
#include "PF_IPS.h"
#include "SIRFilter.h"
#include "Visualization.h"
#include "windows.h"

#include <thread>
#include <cmath>
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
	visualize = true;
	TIME = -1;
	nThreads = 5;
	maxmethod = MAXMETHOD_WEIGHTED;

	viz_frames_dir.assign("../data/frames/");
	// defaults
	use_rssi = true; rssiparam_fname.assign("../data/4_onetag/rssiparam.conf");
	//use_rssi = true; rssiparam_fname.assign("../data/test/rssiparam.conf");
	nParticles = 600;
	time_interval = 5.0;
	use_trajout = true; trajout_fname.assign("../data/trajout_05.csv");
	use_partout = true; partout_fname.assign("../data/partout.csv");
	use_mappng = true; mappng_fname.assign("../data/floorplan_new.png");
	mappng_bounds_provided = true; minx = -16.011; maxx = 46.85; miny = -30.807; maxy = 15.954;
	mapcache_fname = "../data/mapcache.dat";
	moverwrite = true;
	cellwidth = 0.75;
	gt_ref_X = 1.6;
	gt_ref_Y = 1.8;
	attmax = 3.0;
	attdiff = 1.5;
	reposition_ratio = 0.1; 
	movespeed = 0.5;
	nThreads = 1;
	groundtruth_fname.assign("../data/pos_5.txt");
	//groundtruth_fname.assign("../data/ground_truth.txt");
	simulate = false;

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
	} else if ( opt == "reposition") {
		try {
			reposition_ratio = std::stof(val);
		} catch (exception e ) {
			log_e("Error parsing -reposition");
		}
		return true;	
	}  else if ( opt == "maxmethod") {
		if (val == "mean" ) maxmethod = MAXMETHOD_MEAN;
		else if ( val == "weighted ") maxmethod = MAXMETHOD_WEIGHTED;
		else if ( val == "maxp" ) maxmethod = MAXMETHOD_MAXP;
		else log_e("maxmethod \"%s\" not recognized",val.c_str());

		return true;
	} else if ( opt == "dt" ) {
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
	} else if ( opt == "attenuation" ) {
		const char * gtstr = val.c_str();
		attmax = atof(gtstr);
		if (!(gtstr = strchr(gtstr,','))) goto atterror;
		attdiff = atof(++gtstr);
		return true;	
atterror:
		error=1;
		log_e("Error in attenuation string");
		return true;
	} else if ( opt == "trajout") {
		trajout_fname = val;
		use_trajout = true;
		return true;
	} else if ( opt == "frames") {
		viz_frames_dir = val;
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
		if (!(gtstr = strchr(gtstr,','))) goto gterror;
		gt_ref_Y = atof(++gtstr);
		return true;
gterror:
		error = 1;
		log_e("Error in gtorigin string");
		return true;
	} else if ( opt == "simulate") {
		simulate = true;
		return true;
	} else if ( opt == "novis") {
		visualize = false;
		return true;
	} else if ( opt == "nthreads") {
		nThreads = std::stoi(val);
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
		string calibration_data = rssi_config[string("sensors")][string("calibration_data")].asString();

		vector<vector<TimeSeries *>> allrssi;
		int sidx = 0;
		for ( vector<ConfigurationValue>::iterator iter = rssi_config[string("sensors")][string("active_list")].begin(); iter != rssi_config[string("sensors")][string("active_list")].end(); iter++) {
			string id = (*iter).asString();
			RSSISensor sense;
			sense.IDstr = id;
			string posstr = rssi_config[string("sensors")][string("pos_")+id].asString();
			sense.pos.y = -std::stof(posstr.substr(0,posstr.find(' ')))  + sense_ref_Y;
			sense.pos.x = std::stof(posstr.substr(posstr.rfind(' ')+1)) + sense_ref_X;
			log_i("RFID sensor %s at (%.2f, %.2f)",sense.IDstr.c_str(),sense.pos.x,sense.pos.y);

#ifdef GAUSS_MODE
			vector<TimeSeries *> gaussts = CSVLoader::loadMultiTSfromCSV(calibration_data+string("/gaussparams_s")+id+string(".csv"));
			sense.gauss_calib_r = gaussts[0]->t;
			sense.gauss_calib_mu = gaussts[0]->v;
			sense.gauss_calib_sigma = gaussts[1]->v;
			delete gaussts[0];
			delete gaussts[1];
#endif
#ifdef RCELL_MODE
			vector<TimeSeries *> rcellts = CSVLoader::loadMultiTSfromCSV(calibration_data+string("/rcellparams_s")+id+string(".csv"));
			sense.rcell_calib_x = rcellts[0]->t;
			sense.rcell_calib_x.erase(sense.rcell_calib_x.begin());
			for ( int c = 0 ; c < rcellts.size(); c++ ) {
				sense.rcell_calib_r.push_back(rcellts[c]->v[0]);
				rcellts[c]->v.erase(rcellts[c]->v.begin());
				sense.rcell_calib_f.push_back(rcellts[c]->v);
				delete rcellts[c];
			}
#endif

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
		int maxi,maxj;
		for ( int i = 0; i < allrssi.size(); i++) {
			for ( int j = 0; j < allrssi[i].size(); j++) {
				if ( allrssi[i][j] == NULL || allrssi[i][j]->t.size() == 0 ) continue;
				if ( allrssi[i][j]->t[0] < mint ) {
					mint = allrssi[i][j]->t[0];
				}
				if ( allrssi[i][j]->t.back() > maxt ) {
					maxt = allrssi[i][j]->t.back();
					maxi = i;
					maxj = j;
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

			for ( int c = 0; c < gtdata[0]->t.size(); c++ ) {
				log_i("\t\t%.1f %.2f %.2f",gtdata[0]->t[c],gtdata[0]->v[c],gtdata[1]->v[c]);
			}

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
		int TOTAL_POINTS = 0;
		for ( int i = 0; i < allrssi.size(); i++ ) {
			if ( simulate ) {
				log_i("Simulating data for sensor %d",i);
				
				TimeSeries * ts = gtx->copy();
				for ( size_t ti = 0; ti < T.size(); ti++) {
					xycoords gtpos = {gtx->v[ti],gty->v[ti]};
					xycoords spos = sensors[i].pos;
					double distance = dist(gtpos,spos);

#ifdef GAUSS_MODE
					double mu,sigma;
					_get_gaussian_parameters(&(sensors[i]),distance,mu,sigma);
					ts->v[ti] = stdnorm(rengine) * sigma + mu;
#endif
#ifdef RCELL_MODE
					RSSISensor * sensor = &(sensors[i]);
					size_t c = 0,d;
					double val,cumsum;
					for ( c = 0; c < sensor->rcell_calib_r.size(); c++ ) {
						if ( sensor->rcell_calib_r[c] > distance ) {
							double fsum = 0.0;
							for ( d = 0; d < sensor->rcell_calib_f[c].size(); d++ ) fsum += sensor->rcell_calib_f[c][d];
							val = randDouble() * fsum;
							cumsum = 0.0;
							for (  d = 0; d < sensor->rcell_calib_f[c].size(); d++ ) {
								cumsum += sensor->rcell_calib_f[c][d];
								if ( cumsum >= val ) {
									ts->v[ti] = sensor->rcell_calib_x[d];
									break;
								}
							}
							if ( d == sensor->rcell_calib_f[c].size() ) {
								log_i("Last point picked (probably not good)");
								ts->v[ti] = sensor->rcell_calib_x.back();
							}
							break;
						}
					}
					if ( c == sensor->rcell_calib_r.size() ) {
						log_i("\tDistance %.2f too big",distance);
					}
					val = val;
					
#endif
				}
				rssi_data.push_back(ts);
			} else {
				log_i("Interpolating data for sensor %d",i);
			
				int n = 0;
				bool init = false;
				for ( int j = 0; j < allrssi[i].size(); j++ ) {
					if ( allrssi[i][j] == NULL || allrssi[i][j]->t.size() < 5 ) continue;
					TOTAL_POINTS += allrssi[i][j]->t.size();
					if ( !init ) {
						rssi_data.push_back(allrssi[i][j]->interp(T));
						init = true;
					} else {
						*(rssi_data.back()) += *(allrssi[i][j]->interp(T));
					}
					n++;
				}

				if ( n == 0 ) {
					rssi_data.push_back(new TimeSeries(T,std::numeric_limits<double>::quiet_NaN()));
				} else {
					*(rssi_data.back()) /= n;
				}
			}
			
		}
		log_i("Loaded %d raw RSS measurements",TOTAL_POINTS);
		
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
	
	if ( trajout_fname != "" ){
		vector<TimeSeries *> trajts;
		TimeSeries * xts = new TimeSeries();
		TimeSeries * yts = new TimeSeries();
		for ( int c = 0; c < best_state_history.size(); c++ ) {
			xts->insertPointAtEnd(best_state_history_times[c],best_state_history[c].pos.x);
			yts->insertPointAtEnd(best_state_history_times[c],best_state_history[c].pos.y);
		}
		trajts.push_back(xts);
		trajts.push_back(yts);
		CSVLoader::writeMultiTStoCSV(trajout_fname,trajts);
		delete(xts);
		delete(yts);
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
		  "\t-reposition: Reposition ratio\n"
		  "\t-movespeed : Move Speed in m/s\n"
		  "\t-dt        : time interval in s\n"
		  "\t-maxmethod : One of {mean,weighted,maxp}\n"
		  "\t-attenuation : Attenuation constants Format: maxatt,attfactor\n"
		  "\n"
		  "\t-groundtruth : Ground truth CSV\n"
		  "\t-gtorigin	  : Origin of the ground truth\n"
		  "\t-simulate	  : Generate simulated measurements using groundtruth\n"
		  "\t-novis       : Don't start visualizer\n"
		  "\t-nthreads    : Number of execution threads\n"
		  "\n"
		  "\t-trajout   : Max-likelihood state output file\n"
		  "\t-partout   : Particle state estimate and weights\n"
		  "\t-frames	: Output frames for visualization\n"
		  );

	AbstractProgram::printHelp();
}



int _tmain(int argc, _TCHAR* argv[])
{
	Visualization * viz;
	// initialize random seed
	srand (GetTickCount());
	//srand(999);

	PF_IPS prog;
	
	prog.parseCL( argc, argv );
	if ( error ) {
		return error;
	}

	if ( prog.visualize ) {
		viz = new Visualization(&prog);
		if ( !viz->start() ) {
			log_e("Error: Could not start visualization");
			return -1;
		}
	}

	prog.start();	

	if (prog.visualize)
		delete viz;
	return error;
}

