#include "all.h"
#include "ShortestPathMap.h"

ShortestPathMap::ShortestPathMap(const HexMap h) : hexmap(h)
{
}


ShortestPathMap::~ShortestPathMap(void)
{
}


hexcoords ShortestPathMap::idx2Hex(unsigned int idx) {
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
error1:
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

ShortestPathMap * ShortestPathMap::generateFromObstacleMap(const vector<vector<bool>> &hexdata, const string pngname, const HexMap hmap, int hexmovespeed) {
	ShortestPathMap * retval = new ShortestPathMap(hmap);
	retval->pngfname = pngname;
	retval->hexmovespeed = hexmovespeed;

	int nidx = hmap.getRows() * hmap.getColumns();
	int ** dist = new int*[nidx];
	if ( !dist ) {
		log_e("Error allocating calculation matrices");
		return NULL;
	}
	for ( int c = 0; c < nidx; c++ ) {
		dist[c] = new int[nidx];
		if ( !dist[c]) {
			log_e("Error allocating calculation matrices");
			return NULL;
		}
		for ( int d = 0; d < nidx; d++ ) {
			if ( c == d ) {
				dist[c][d] = 0;
			} else {
				dist[c][d] = -1;
			}
		}
	}

	// initialize "edges"
	for (int c = 0; c < nidx; c++ ) {
		hexcoords hc = retval->idx2Hex(c);
		if ( hc.i > 0 ) {
			dist[c][c-1] = 1;  // (-1,0)
			if ( hc.j > 0 && !(hc.i % 2))
				dist[c][c - 1 - hmap.getColumns()] = 1; // (-1,-1)
			else if ( hc.j < hmap.getRows() - 1 && (hc.i % 2))
				dist[c][c - 1 + hmap.getColumns()] = 1; // (-1,+1)
		}

		if ( hc.j > 0 )
			dist[c][c-hmap.getColumns()] = 1; //(0,-1)
		if ( hc.j < hmap.getRows() - 1 )
			dist[c][c+hmap.getColumns()] = 1; //(0,+1)

		if ( hc.i < hmap.getColumns() - 1 ) {
			dist[c][c+1] = 1; // (+1,0)
			if ( hc.j > 0 && !(hc.i % 2) )
				dist[c][c + 1 - hmap.getColumns()] = 1; // (+1,-1)
			else if ( hc.j < hmap.getRows() - 1 && (hc.i % 2) )
				dist[c][c + 1 + hmap.getColumns()] = 1; // (+1,+1)
		}
	}

	// remove all edges to/from obstacles
	for ( int row = 0; row < hmap.getRows(); row++ ) {
		for ( int col = 0; col < hmap.getColumns(); col++ ) {
			if ( hexdata[row][col] ) {
				for ( int i = 0; i < nidx; i++ ) {
					dist[col + row*hmap.getColumns()][i] = dist[i][col + row*hmap.getColumns()] = -1;
				}
			}
		}
	}

	//ready for (limited) floyd warshall
	for (int k = 0; k < nidx; k++ ) {
		for ( int i = 0; i < nidx; i++ ) {
			if ( i != k && dist[i][k] < hexmovespeed && dist[i][k] != -1 )
				for ( int j = 0; j < nidx; j++ ) {
					if ( (dist[i][k] + dist[k][j] < dist[i][j] || dist[i][j] == -1) && dist[i][k] + dist[k][j] <= hexmovespeed && dist[k][j] != -1)
						dist[i][j] = dist[i][k] + dist[k][j];
				}
		}
	}

	// now populate cache
	retval->map.reserve(nidx);
	for ( int k = 0; k < nidx; k++ ) {
		vector<int> subvec;
		for ( int j = 0; j < nidx; j++ ) {
			if ( dist[k][j] != -1 ) {
				subvec.push_back(j);
			}
		}
		subvec.shrink_to_fit();
		retval->map.push_back(subvec);
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
