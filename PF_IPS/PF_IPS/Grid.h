#pragma once
#include "all.h"

#include "PFTypes.h"

//See: http://blog.ruslans.com/2011/02/hexagonal-grid-math.html

#define XMIN 0
#define XMAX 1
#define YMIN 2
#define YMAX 3

class Grid
{
public:
	Grid();
	Grid( const double pbounds[4], double hexradius, double nHexOvercoverage );
	~Grid(void);

	bool operator==(const Grid &other) const;
	bool operator!=(const Grid &other) const;

	int getRows() const;
	int getColumns() const;

	xycoords getCenter( gridcoords cell ) const;
	gridcoords nearestCell( xycoords xy ) const;

	vector<vector<double>> interpolateXYData( const vector<vector<double>> &data, const vector<double> &xpoints, const vector<double> &ypoints ) const;
	vector<vector<double>> interpolateXYData( const vector<vector<double>> &data, const double dbounds[4]) const;

	string toString();

public:
	double bounds[4];
	double H,R,S,W;

	int rows, cols;

};
