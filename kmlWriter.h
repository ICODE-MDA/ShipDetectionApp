#ifndef KMLWRITER_H_
#define KMLWRITER_H_
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>

#include <ossim/base/ossimGeoPolygon.h>

using namespace std;

class kmlWriter{
public:
	kmlWriter();
	bool isGood();
	~kmlWriter();
	void open(string filename);
	void writePlacemark(string name, string description, double lat, double lon, double height);
	void writePath(string name, string description, vector< vector<double> >& coordinates, int extrude =1, int tessellate =1, string altitude = "absolute");
	void writePolygon(string name, string description, ossimGeoPolygon& coordinates, int extrude =1, int tessellate =1, string altitude = "absolute");
private:
	void writeHeader();
	void writeFooter();

	string _filename;
	ofstream _stream;
};



#endif /* KMLWRITER_H_ */
