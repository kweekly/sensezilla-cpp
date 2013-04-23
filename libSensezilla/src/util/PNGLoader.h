#ifndef PNG_LOADER_H
#define PNG_LOADER_H

typedef struct {
	int width, height;
	double ** R,**G,**B,**A;
	double ** GRAY;
} PNGData;

class PNGLoader {
public:
	static PNGData loadFromPNG(string fname);
	static void writeToPNG(string fname, PNGData data);
	static void freePNGData(PNGData * data);

private:
	static double ** allocate2D(int width, int height);
	static void free2D(double ** dat, int height);
};




#endif