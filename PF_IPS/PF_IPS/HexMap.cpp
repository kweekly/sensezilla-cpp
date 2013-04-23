#include "HexMap.h"

HexMap::HexMap() {
	R = H = S = W = 0;
}

HexMap::HexMap( double bounds[4], double hexradius, double nHexOvercoverage  ) {
	bounds[XMIN] -= nHexOvercoverage * hexradius / 2;
	bounds[XMAX] += nHexOvercoverage * hexradius / 2;
	bounds[YMIN] -= nHexOvercoverage * hexradius / 2;
	bounds[YMAX] += nHexOvercoverage * hexradius / 2;

	for ( int c = 0; c < 4; c++ ) this->bounds[c] = bounds[c];
	this->R = hexradius;
	this->H = 2*R*sin(60*M_PI / 180);
	this->S = 3/2*R;
	this->W = 2*R;

	cols = (int)(((bounds[XMAX] - bounds[XMIN]) - W)/S) + 1;
	rows = (int)(((bounds[YMAX] - bounds[YMIN]) - H/2)/H) + 1;
	if ( cols <= 0 || rows <= 0 ) {
		log_e("Error: Hex tiles are to big to fit in x/y bounds!");
	}
}

HexMap::~HexMap(void) {
}


bool HexMap::operator==(const HexMap &other) const {
	return rows == other.rows && cols == other.cols && H == other.H && R == other.R && S == other.S && W == other.W &&
			bounds[0] == other.bounds[0] && bounds[1] == other.bounds[1]  && bounds[2] == other.bounds[2]  && bounds[3] == other.bounds[3] ;
}
bool HexMap::operator!=(const HexMap &other) const {
	return !(*this == other);
}

int HexMap::getRows() {
	return rows;
}

int HexMap::getColumns() {
	return cols;
}

xycoords HexMap::getCenter( hexcoords hex ) {
	xycoords retval;
	retval.y = hex.j * H - ((hex.i+1)%2)*H/2 - H/2 + bounds[YMIN];
	retval.x = hex.i * S + R + bounds[XMIN];
	return retval;
}

hexcoords HexMap::nearestHex(xycoords xy ) {
	hexcoords retval;
	retval.i = (int)((xy.x - bounds[XMIN])/S);
	retval.j = (int)ceil((xy.y - bounds[YMIN] + (retval.i%2)*H/2 + H/2)/H);
	return retval;
}

vector<vector<double>> HexMap::interpolateXYData( const vector<vector<double>> &data, const vector<double> &xpoints, const vector<double> &ypoints ) {
	vector<vector <double>> retv(rows, vector<double> (cols, 0.0));
	vector<vector <int>> retc(rows, vector<int>(cols, 0.0));

	for ( int xi = 0; xi < xpoints.size(); xi++ ) {
		for ( int yi = 0; yi < ypoints.size(); yi++  ) {
			xycoords xy = {xpoints[xi],ypoints[yi]};
			hexcoords hex = nearestHex( xy );

			retc[hex.j][hex.i] ++;
			retv[hex.j][hex.i] += data[yi][xi];
		}
	}

	for ( int j = 0; j < rows; j++ ) {
		for ( int i = 0; i < cols; i++) {
			if ( retc[j][i] > 0 )
				retv[j][i] /= retc[j][i];
		}
	}

	return retv;
}