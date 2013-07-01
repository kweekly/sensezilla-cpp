#include "all.h"
#include "ShortestPathMap.h"

ShortestPathMap::ShortestPathMap(const HexMap h) : hexmap(h)
{
}


ShortestPathMap::~ShortestPathMap(void)
{
}

hexcoords ShortestPathMap::getRandomHexInRange( hexcoords start ) {
	if ( start.i < 0 || start.i >= hexmap.getColumns() || start.j < 0 || start.j >= hexmap.getRows()) {
		start.i = start.j = -1;
		return start;
	}
	int startidx = hex2idx(start);

	if ( map[startidx].size() == 0 ) {
		start.i = start.j = -1;
		return start;
	}
	int endno = rand() % ( map[startidx].size() );
	int endidx = map[startidx][endno];
	return idx2hex(endidx);
}


hexcoords ShortestPathMap::idx2hex(unsigned int idx) {
	hexcoords ret;
	ret.i = idx % hexmap.getColumns();
	ret.j = idx / hexmap.getColumns();
	return ret;
}

unsigned int ShortestPathMap::hex2idx(hexcoords hex) {
	return hex.i + hex.j * hexmap.getColumns();
}

bool ShortestPathMap::_loadCacheParams(FILE * fin, string *pngname, HexMap* hmap, int *hexmovespeed) {
	int rd;
	int l;

	// filename
	char fnamebuf[128];
	rd = fread(&l,sizeof(l),1,fin);
	if ( rd != 1 ) return false;
	if ( l >= 128 ) {
		log_e("Error: Length of name too long in cache file");
		return false;
	}

	rd = fread(fnamebuf,1,l,fin);
	fnamebuf[l] = 0;
	pngname->assign(fnamebuf);
	if ( rd != l ) return false;

	// hexmap
	rd = fread(hmap, sizeof(HexMap),1,fin);
	if ( rd != 1 ) return false;
	
	// hexmovespeed
	rd = fread(hexmovespeed, sizeof(int),1,fin);
	if ( rd != 1 ) return false;

	return true;
}

bool ShortestPathMap::_loadCacheData(FILE * fin, ShortestPathMap * retval) {
	int l;
	int rd;

	// load data
	rd = fread(&l,sizeof(l),1,fin);
	if ( rd != 1 ) goto rderr2;
	retval->map.reserve(l);
	for ( int c = 0; c < l; c++ ) {
		int m;
		rd = fread(&m,sizeof(m),1,fin);
		if ( rd != 1 ) goto rderr2;
		vector<int> subvec(m);
		for ( int d = 0; d< m; d++ ) {
			int n;
			rd = fread(&n, sizeof(n),1,fin);
			if ( rd != 1) goto rderr2;
			subvec[d] = n;
		}
		subvec.shrink_to_fit();
		retval->map.push_back(subvec);
	}
	retval->map.shrink_to_fit();

	return true;
rderr2:
	return false;
}

ShortestPathMap * ShortestPathMap::loadFromCache(const string fname) {
	string pngname;
	HexMap hmap;
	int hexmovespeed;

	FILE * fin = fopen(fname.c_str(),"rb");
	if ( !fin ) {
		log_e("Error: Cannot open cache file");
		return NULL;
	}
	
	if (!_loadCacheParams(fin,&pngname,&hmap,&hexmovespeed)) goto rderr;
	{
	string hmap_str = hmap.toString();
	log_i("Parameters found in map cache:");
	log_i("\tGenerated from: %s",pngname.c_str());
	log_i("\tBounds: %s",hmap_str.c_str());
	log_i("\tMovespeed (hexes): %d",hexmovespeed);
	}

	ShortestPathMap * retval = new ShortestPathMap(hmap);
	retval->pngfname = pngname;
	retval->hexmovespeed = hexmovespeed;

	if ( !_loadCacheData(fin,retval) ) goto rderr2;

	fclose(fin);
	return retval;
rderr2:
	delete retval;
rderr:
	log_e("Read Error. Abort.");

	fclose(fin);
	return NULL;
}

ShortestPathMap * ShortestPathMap::loadFromCache(const string fname, const string pngname, const HexMap hmap, int hexmovespeed) {
	FILE * fin = fopen(fname.c_str(),"rb");
	if ( !fin ) {
		log_e("Error: Cannot open cache file");
		return NULL;
	}
		
	string tname;
	HexMap thmap;
	int thms;

	if (!_loadCacheParams(fin,&tname,&thmap,&thms)) goto rderr;

	if( pngname != tname ) {
		log_e("Error: Cache was generated with %s but pngname is %s",tname.c_str(),pngname.c_str());
		goto error1;
	}

	if ( thmap != hmap ) {
		log_e("Error: Cache was generated with different hexmap parameters");
		goto error1;
	}

	if ( thms != hexmovespeed ) {
		log_e("Error: Cache was generated with different movespeed parameter");
		goto error1;
	}

	ShortestPathMap * retval = new ShortestPathMap(hmap);
	retval->pngfname = pngname;
	retval->hexmovespeed = hexmovespeed;

	if ( !_loadCacheData(fin,retval) ) goto rderr2;

	fclose(fin);
	return retval;
rderr2:
	delete retval;
rderr:
	log_e("Read Error. Abort.");
error1:
	fclose(fin);
	return NULL;
}

void ShortestPathMap::_addCheckObs( const vector<vector<bool>> &hexdata, int columns, set<int> *set, int hi, int hj ) {
		if ( hi >= 0 && hi < (int)(hexdata[0].size()) && hj > 0 && hj < (int)(hexdata.size()) && !hexdata[hj][hi] ) {
			set->insert(hj*columns + hi);
		}
}

set<int> ShortestPathMap::_nearestHexes( const vector<vector<bool>> &hexdata, int position, const HexMap & hmap, int hexmovespeed ) {
	set<int> tocheck,retval;
	set<int> candidates;

	// check if currently at obstacle or out of range
	int ci = position % hmap.getColumns();
	int cj = position / hmap.getColumns();
	if ( ci < 0 || ci >= (int)(hexdata[0].size()) || cj < 0 || cj >= (int)(hexdata.size()) || hexdata[cj][ci] ) {
		return retval;
	}

	tocheck.insert(position);
	for ( int c = 0; c < hexmovespeed; c++ ) {
		candidates.clear();
		// add all neighboring nodes
		for (set<int>::iterator i = tocheck.begin(); i != tocheck.end(); i++) {
			int pcheck = * i;
			int hi = pcheck % hmap.getColumns();
			int hj = pcheck / hmap.getColumns();
			if (hi % 2) { // odd
				_addCheckObs(hexdata, hmap.getColumns(), &candidates, hi + 1, hj - 1 );
				_addCheckObs(hexdata, hmap.getColumns(), &candidates, hi - 1, hj - 1 );
			} else { // even
				_addCheckObs(hexdata, hmap.getColumns(), &candidates, hi + 1, hj + 1);
				_addCheckObs(hexdata, hmap.getColumns(), &candidates, hi - 1, hj + 1 );
			}
			_addCheckObs(hexdata, hmap.getColumns(), &candidates, hi - 1, hj );
			_addCheckObs(hexdata, hmap.getColumns(), &candidates, hi , hj - 1 );
			_addCheckObs(hexdata, hmap.getColumns(), &candidates, hi + 1, hj );
			_addCheckObs(hexdata, hmap.getColumns(), &candidates, hi, hj + 1 );			

			// retval = union(retval, tocheck)
			retval.insert(pcheck);
		}

		// tocheck = diff(candidates,retval)
		tocheck.clear();
		set_difference(candidates.begin(),candidates.end(),retval.begin(),retval.end(),std::inserter(tocheck,tocheck.end()));
	}
	return retval;	
}

ShortestPathMap * ShortestPathMap::generateFromObstacleMap(const vector<vector<bool>> &hexdata, const string pngname, const HexMap hmap, int hexmovespeed) {
	ShortestPathMap * retval = new ShortestPathMap(hmap);
	retval->pngfname = pngname;
	retval->hexmovespeed = hexmovespeed;

	int nidx = hmap.getRows() * hmap.getColumns();

	// calculate cache
	retval->map.reserve(nidx);
	for ( int k = 0; k < nidx; k++ ) {
		if ( k % 10000 == 0 ) {
			log_i("%d of %d cells completed",k,nidx);
		}
		vector<int> donevec;
		set<int> nset = _nearestHexes(hexdata, k, hmap, hexmovespeed);
		donevec.assign(nset.begin(),nset.end());
		donevec.shrink_to_fit();
		retval->map.push_back(donevec);
	}
	retval->map.shrink_to_fit();
	return retval;
}

void ShortestPathMap::saveToCache(string fname) {
	FILE * fout = fopen(fname.c_str(),"wb");
	const char * fname_c = pngfname.c_str();
	int l = strlen(fname_c);

	// write filename that was used to generate file
	fwrite(&l,sizeof(l),1,fout); // write length of string
	fwrite(fname_c,1,l,fout); // write string itself

	// write hexgrid parameters
	fwrite(&hexmap,sizeof(hexmap),1,fout);

	// write hexmovespeed
	fwrite(&hexmovespeed,sizeof(hexmovespeed),1,fout);

	// write data
	l = map.size();
	fwrite(&l,sizeof(l),1,fout);
	for ( int c = 0; c < l; c++ ) {
		int m = map[c].size();
		fwrite(&m,sizeof(m),1,fout);
		for ( int d = 0; d < m; d++ ) {
			int n = map[c][d];
			fwrite(&n, sizeof(n),1,fout);
		}
	}

	fclose(fout);
}
