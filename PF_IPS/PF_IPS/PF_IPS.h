#pragma once
#include "all.h"
#include "PFTypes.h"
#include "ShortestPathMap.h"



class PF_IPS : public AbstractProgram {
public:
	PF_IPS();
	~PF_IPS();
	
	bool processCLOption( string opt, string val );
	void start();
	void printHelp();

private:
	// Algorithms
	void _calibrate_RSSI_system();
	
	
	
	void _loadRSSIData();
	void _loadXYData();
	void _loadMapPNG();
	

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

	// data members
	map<string,map<string, ConfigurationValue>> xy_config;	
	map<string,map<string, ConfigurationValue>> rssi_config;
	vector<RSSISensor> sensors;
	vector<RSSITag> reference_tags;
	vector<vector<TimeSeries *>> rssi_refdata;
	ShortestPathMap * pathmap;

	string rssi_basedir;

	double tag_ref_X;
	double tag_ref_Y;
	double tag_ref_Z;

	double sense_ref_X;
	double sense_ref_Y;
	double sense_ref_Z;
};


class State {
public:
	double x;
	double y;
};

class Observation {
	vector<double> rfid_system_rssi_measurements;
};

class Params {
	PF_IPS * context;
};