#ifndef PF_TYPES_H
#define PF_TYPES_H

struct xycoords {
	double x,y;
};

struct gridcoords {
	int i,j;
};


struct RSSISensor {
	string IDstr;
	xycoords pos;

	
	vector<double> gauss_calib_r;
	vector<double> gauss_calib_mu;
	vector<double> gauss_calib_sigma;
	
};

class PF_IPS;

class State {
public:
	xycoords pos;
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