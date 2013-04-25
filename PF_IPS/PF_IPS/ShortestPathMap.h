#pragma once
#include "all.h"
#include "HexMap.h"


class ShortestPathMap
{
public:
	// returns NULL if map cache does not satisfy parameters
	static ShortestPathMap * loadFromCache(const string fname, const string pngname, const HexMap hmap, int hexmovespeed);
	// loads all parameters from cache
	static ShortestPathMap * loadFromCache(const string fname);

	static ShortestPathMap * generateFromObstacleMap(const vector<vector<bool>> &hexdata, const string pngname, const HexMap hmap, int hexmovespeed);

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

	static set<int> _nearestHexes( const vector<vector<bool>> &hexdata, int position, const HexMap & hmap, int hexmovespeed );
	static void _addCheckObs( const vector<vector<bool>> &hexdata, int columns, set<int> *set, int hi, int hj);

	static bool _loadCacheParams(FILE * fin, string *pngname, HexMap* hmap, int *hexmovespeed);
	static bool _loadCacheData(FILE * fin, ShortestPathMap * ptr);

	ShortestPathMap(const HexMap h);
	~ShortestPathMap(void);

};

