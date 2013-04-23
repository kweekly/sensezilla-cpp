#pragma once
#include "all.h"
#include "HexMap.h"


class ShortestPathMap
{
public:
	// returns NULL if map cache does not satisfy parameters
	static ShortestPathMap * loadFromCache(const string fname, const string pngname, const HexMap hmap, int hexmovespeed);

	static ShortestPathMap * generateFromObstacleMap(vector<vector<double>> hexdata, const string pngname, const HexMap hmap, int hexmovespeed);

	void saveToCache(const string fname);

	
	
private:
	//parameteres
	HexMap hexmap;
	string pngfname;
	int hexmovespeed;
	
	// data
	vector<vector<int>> map;
	
	
	hexcoords idx2Hex(unsigned int idx);
	unsigned int hex2idx(hexcoords hex);

	ShortestPathMap(const HexMap h);
	~ShortestPathMap(void);

};

