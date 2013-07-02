#pragma once
#include "all.h"
#include "Grid.h"


class ShortestPathMap
{
public:
	// returns NULL if map cache does not satisfy parameters
	static ShortestPathMap * loadFromCache(const string fname, const string pngname, const Grid grid, int cellmovespeed);
	// loads all parameters from cache
	static ShortestPathMap * loadFromCache(const string fname);

	static ShortestPathMap * generateFromObstacleMap(const vector<vector<bool>> &hexdata, const string pngname, const Grid grid, int cellmovespeed);

	void saveToCache(const string fname);

	gridcoords getRandomCellInRange( gridcoords start );

		~ShortestPathMap(void);
private:
	//parameteres
	Grid grid;
	string pngfname;
	int cellmovespeed;
	
	// data
	vector<vector<int>> map;	
	
	gridcoords idx2hex(unsigned int idx);
	unsigned int hex2idx(gridcoords cell);

	static set<int> _nearestHexes( const vector<vector<bool>> &hexdata, int position, const Grid & grid, int cellmovespeed );
	static void _addCheckObs( const vector<vector<bool>> &hexdata, int columns, set<int> *set, int hi, int hj);

	static bool _loadCacheParams(FILE * fin, string *pngname, Grid* grid, int *cellmovespeed);
	static bool _loadCacheData(FILE * fin, ShortestPathMap * ptr);

	ShortestPathMap(const Grid h);


};

