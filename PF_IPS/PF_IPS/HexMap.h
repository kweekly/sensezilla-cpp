#pragma once
#include "all.h"

#include "PFTypes.h"

//See: http://blog.ruslans.com/2011/02/hexagonal-grid-math.html

#define XMIN 0
#define XMAX 1
#define YMIN 2
#define YMAX 3

class HexMap
{
public:
	HexMap();
	HexMap( double bounds[4], double hexradius, double nHexOvercoverage );
	~HexMap(void);

	bool operator==(const HexMap &other) const;
	bool operator!=(const HexMap &other) const;

	int getRows();
	int getColumns();

	xycoords getCenter( hexcoords hex );
	hexcoords nearestHex( xycoords xy );

	vector<vector<double>> interpolateXYData( const vector<vector<double>> &data, const vector<double> &xpoints, const vector<double> &ypoints );

private:
	double bounds[4];
	double H,R,S,W;

	int rows, cols;
};
