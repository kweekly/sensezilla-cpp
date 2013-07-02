#include "Grid.h"

Grid::Grid() {
	R = H = S = W = 0;
}

Grid::Grid( const double pbounds[4], double hexradius, double nHexOvercoverage  ) {

	for ( int c = 0; c < 4; c++ ) this->bounds[c] = pbounds[c];

	this->bounds[XMIN] -= nHexOvercoverage * hexradius * 2;
	this->bounds[XMAX] += nHexOvercoverage * hexradius * 2;
	this->bounds[YMIN] -= nHexOvercoverage * hexradius * 2;
	this->bounds[YMAX] += nHexOvercoverage * hexradius * 2;

	this->R = hexradius;
	this->H = 2*R*sin(60.0*M_PI / 180.);
	this->S = 3.0/2*R;
	this->W = 2.*R;

	cols = (int)((this->bounds[XMAX] - this->bounds[XMIN])/S) + 1;
	rows = (int)(((this->bounds[YMAX] - this->bounds[YMIN]) + H/2)/H) + 1;
	if ( cols <= 0 || rows <= 0 ) {
		log_e("Error: Cell tiles are to big to fit in x/y bounds!");
	}
}

Grid::~Grid(void) {
}


bool Grid::operator==(const Grid &other) const {
	return rows == other.rows && cols == other.cols && H == other.H && R == other.R && S == other.S && W == other.W &&
			bounds[0] == other.bounds[0] && bounds[1] == other.bounds[1]  && bounds[2] == other.bounds[2]  && bounds[3] == other.bounds[3] ;
}
bool Grid::operator!=(const Grid &other) const {
	return !(*this == other);
}

int Grid::getRows() const {
	return rows;
}

int Grid::getColumns() const {
	return cols;
}

xycoords Grid::getCenter( gridcoords cell ) const {
	xycoords retval;
	retval.y = cell.j * H + (abs(cell.i)%2)*H/2 + bounds[YMIN];
	retval.x = cell.i * S + bounds[XMIN];
	return retval;
}

gridcoords Grid::nearestCell(xycoords xy ) const {
	gridcoords retval;
	xy.x -= bounds[XMIN];
	xy.y -= bounds[YMIN];
	retval.i = (int)floor(xy.x/S);
	double yts = (xy.y - (abs(retval.i)%2)*H/2);
	retval.j = (int)floor(yts/H);
	double xt = xy.x - retval.i*S;
	double yt = yts - retval.j*H;
	if (xt <= R*abs(0.5 - yt/H) ){
		retval.i -= 1;
		retval.j += -(abs(retval.i)%2) + (xy.y > H/2)?1:0;
	}
	return retval;
}

vector<vector<double>> Grid::interpolateXYData( const vector<vector<double>> &data, const double dbounds[4] )  const {
	vector<vector <double>> retv(rows, vector<double> (cols, 0.0));
	vector<vector <int>> retc(rows, vector<int>(cols, 0));

	for ( size_t xi = 0; xi < data[0].size(); xi++ ) {
		for ( size_t yi = 0; yi < data.size(); yi++  ) {
			xycoords xy = {dbounds[XMIN] + (double)xi/(data[0].size()-1) * (dbounds[XMAX]-dbounds[XMIN]),
							dbounds[YMIN] + (double)yi/(data.size()-1) * (dbounds[YMAX]-dbounds[YMIN])};
			gridcoords cell = nearestCell( xy );

			if ( cell.j >= rows || cell.i>= cols || cell.j < 0 || cell.i < 0 ) {
				log_e("Error: Vector subscript out of range %d,%d",cell.i,cell.j);
			} else {
				retc[cell.j][cell.i] ++;
				retv[cell.j][cell.i] += data[yi][xi];
			}
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

vector<vector<double>> Grid::interpolateXYData( const vector<vector<double>> &data, const vector<double> &xpoints, const vector<double> &ypoints )  const {
	vector<vector <double>> retv(rows, vector<double> (cols, 0.0));
	vector<vector <int>> retc(rows, vector<int>(cols, 0));

	for ( size_t xi = 0; xi < xpoints.size(); xi++ ) {
		for ( size_t yi = 0; yi < ypoints.size(); yi++  ) {
			xycoords xy = {xpoints[xi],ypoints[yi]};
			gridcoords cell = nearestCell( xy );

			retc[cell.j][cell.i] ++;
			retv[cell.j][cell.i] += data[yi][xi];
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

string Grid::toString() {
	return string("[") + to_string(cols) + "x" + to_string(rows) + " cell grid r=" + to_string(R) + " bounds=["+to_string(bounds[0])+"-"+to_string(bounds[1])+","+to_string(bounds[2])+"-"+to_string(bounds[3]) +"]";
}