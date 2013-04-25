#ifndef PF_TYPES_H
#define PF_TYPES_H

struct xycoords {
	double x,y;
};

struct hexcoords {
	int i,j;
};


struct RSSISensor {
	string IDstr;
	xycoords pos;
};

struct RSSITag { 

	string IDstr;
	int id;
	bool isref;

	xycoords pos;
};

#endif