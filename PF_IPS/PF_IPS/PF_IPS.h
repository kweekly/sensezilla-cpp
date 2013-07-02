#pragma once
#include "all.h"
#include "PFTypes.h"
#include "ShortestPathMap.h"
#include "SIRFilter.h"
#include <mutex>

class PF_IPS : public AbstractProgram {
public:
	PF_IPS();
	~PF_IPS();
	
	bool processCLOption( string opt, string val );
	void start();
	void printHelp();

private:
	friend class Visualization;
	bool req_data_lock;
	mutex data_locked_mutex;
	mutex data_locked_ack_mutex;

	// Algorithms
	SIRFilter<State,Observation,Params> * filter;
	static void transition_model(State & state, Params & params);
	static double observation_model(const State & in_state, const Observation & measurement, Params & params);
	static void _get_gaussian_parameters(RSSISensor * sensor, double distance, double & mu, double & sigma);

	void _sirFilter();

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
	double time_interval;

	bool simulate;

	string rssiparam_fname, xyparam_fname, trajout_fname, partout_fname;
	string mappng_fname, mapcache_fname, statein_fname, stateout_fname;
	string groundtruth_fname;

	// data members
	// Parameters
	map<string,map<string, ConfigurationValue>> xy_config;	
	map<string,map<string, ConfigurationValue>> rssi_config;
	vector<RSSISensor> sensors;

	Grid grid;
	ShortestPathMap * pathmap;

	string rssi_basedir;

	double sense_ref_X;
	double sense_ref_Y;
	double sense_ref_Z;

	vector<TimeSeries *> gtdata;
	double gt_ref_X;
	double gt_ref_Y;

	double TIME;

	// Sensor Data
	vector<TimeSeries *> rssi_data;

};



double tsSTD(TimeSeries *ts);
double dist(xycoords x1, xycoords x2);


