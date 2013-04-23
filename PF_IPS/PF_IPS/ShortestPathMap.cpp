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

ShortestPathMap * ShortestPathMap::loadFromCache(const string fname, const string pngname, const HexMap hmap, int hexmovespeed) {
	FILE * fin = fopen(fname.c_str(),"rb");
	if ( !fin ) {
		log_e("Error: Cannot open cache file");
		return NULL;
	}
	char fnamebuf[128];
	int l;
	int rd;r

	// filename
	rd = fread(&l,sizeof(l),1,fin);
	if ( rd != sizeof(l) ) goto rderr;
	if ( l >= 128 ) {
		log_e("Error: Length of name too long in cache file");
		goto error1;
	}
	rd = fread(fnamebuf,1,l,fin);
	if ( rd != l ) goto rderr;
	fnamebuf[l] = 0;
	if( pngname != fnamebuf ) {
		log_e("Error: Cache was generated with %s but pngname is %s",fnamebuf,pngname.c_str());
		goto error1;
	}

	// hexmap
	HexMap hmt;
	rd = fread(&hmt, sizeof(hmt),1,fin);
	if ( rd != sizeof(hmt) ) goto rderr;
	if ( hmt != hmap ) {
		log_e("Error: Cache was generated with different hexmap parameters");
		goto error1;
	}

	// hexmovespeed
	int hmst;
	rd = fread(&hmst, sizeof(hmst), 1, fin);
	if ( rd != sizeof(hmst) ) goto rderr;
	if ( hmst != hexmovespeed ) {
		log_e("Error: Cache was generated with different movespeed parameter");
		goto error1;
	}

	ShortestPathMap * retval = new ShortestPathMap(hmap);
	retval->pngfname = pngname;
	retval->hexmovespeed = hexmovespeed;

	// load data
	rd = fread(&l,sizeof(l),1,fin);
	if ( rd != sizeof(l) ) goto rderr2;
	retval->map.reserve(l);
	for ( int c = 0; c < l; c++ ) {
		int m;
		rd = fread(&m,sizeof(m),1,fin);
		if ( m != sizeof(m) ) goto rderr2;
		vector<int> subvec;
		subvec.reserve(m);
		for ( int d = 0; d< m; d++ ) {
			int n;
			rd = fread(&n, sizeof(n),1,fin);
			if ( n != sizeof(n)) goto rderr2;
			subvec[d] = n;
		}
		subvec.shrink_to_fit();
		retval->map.push_back(subvec);
	}
	retval->map.shrink_to_fit();

	return retval;
rderr2:
	delete retval;
rderr:
	log_e("Read Error. Abort.");
error1:
	fclose(fin);
	return NULL;
}

ShortestPathMap * ShortestPathMap::generateFromObstacleMap(vector<vector<double>> hexdata, const string pngname, const HexMap hmap, int hexmovespeed) {
	ShortestPathMap * retval = new ShortestPathMap(hmap);
	retval->pngfname = pngname;
	retval->hexmovespeed = hexmovespeed;
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
