#include "Grid.h"

Grid::Grid() {
	W = 0;
}

Grid::Grid( const double pbounds[4], double cellwidth, double nCellsOvercoverage  ) {

	for ( int c = 0; c < 4; c++ ) this->bounds[c] = pbounds[c];

	this->bounds[XMIN] -= nCellsOvercoverage * cellwidth;
	this->bounds[XMAX] += nCellsOvercoverage * cellwidth;
	this->bounds[YMIN] -= nCellsOvercoverage * cellwidth;
	this->bounds[YMAX] += nCellsOvercoverage * cellwidth;

	this->W = cellwidth;

	cols = (int)((this->bounds[XMAX] - this->bounds[XMIN])/W) + 1;
	rows = (int)((this->bounds[YMAX] - this->bounds[YMIN])/W) + 1;
	if ( cols <= 0 || rows <= 0 ) {
		log_e("Error: Cell tiles are to big to fit in x/y bounds!");
	}
}

Grid::~Grid(void) {
}


bool Grid::operator==(const Grid &other) const {
	return rows == other.rows && cols == other.cols && W == other.W &&
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
	retval.y = W*cell.j + bounds[YMIN] + W/2;
	retval.x = W*cell.i + bounds[XMIN] + W/2;
	return retval;
}

gridcoords Grid::nearestCell(xycoords xy ) const {
	gridcoords retval;
	retval.i = floor((xy.x - bounds[XMIN]) / W);
	retval.j = floor((xy.y - bounds[YMIN]) / W);
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
	return string("[") + to_string(cols) + "x" + to_string(rows) + " cell grid w=" + to_string(W) + " bounds=["+to_string(bounds[0])+"-"+to_string(bounds[1])+","+to_string(bounds[2])+"-"+to_string(bounds[3]) +"]";
}