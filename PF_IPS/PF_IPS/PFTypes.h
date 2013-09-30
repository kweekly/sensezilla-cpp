#ifndef PF_TYPES_H
#define PF_TYPES_H

//#define GAUSS_MODE
#define RCELL_MODE


struct xycoords {
	double x,y;
};

struct gridcoords {
	int i,j;
};


struct RSSISensor {
	string IDstr;
	xycoords pos;

#ifdef GAUSS_MODE
	vector<double> gauss_calib_r;
	vector<double> gauss_calib_mu;
	vector<double> gauss_calib_sigma;
#endif

#ifdef RCELL_MODE
	vector<double> rcell_calib_r;
	vector<double> rcell_calib_x;
	vector<vector<double>> rcell_calib_f;
#endif
	
};

class PF_IPS;

class State {
public:
	xycoords pos;
	double attenuation;
};

class Observation {
public:
	vector<double> rfid_system_rssi_measurements;
};

class Params {
public:
	PF_IPS * context;
};

#endif