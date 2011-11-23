#ifndef shipDetectionFilter_H_
#define shipDetectionFilter_H_

#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/imaging/ossimImageCombiner.h>
#include <ossim/base/ossimGeoPolygon.h>

#include <shapelib/shapefil.h>

#include <vector>

#include <opencv/cv.h>
#include <opencv/highgui.h>

// Include cvBlobs header files
#include "blob.h"
#include "BlobResult.h"

#include "kmlWriter.h"
//#include "shpWriter.h"

// Declare shipDetectionFilter class
// This class is based on the ossimImageCombiner class
class shipDetectionFilter : public ossimImageCombiner
{
public:
	// Constructors
	shipDetectionFilter(ossimObject *owner=NULL);
	// Destructor
	virtual ~shipDetectionFilter();
	// Short and long names for filter
	ossimString getShortName()const
	{
		return ossimString("Tile to IplImage Filter");
	}

	ossimString getLongName()const
	{
		return ossimString("Tile to IplImage Filter");
	}

	// getTile function replaces getTile in ossimImageCombiner
	virtual ossimRefPtr<ossimImageData> getTile(const ossimIrect& tileRect, ossim_uint32 resLevel=0);

	virtual void initialize();

	virtual ossimScalarType getOutputScalarType() const;

	ossim_uint32 getNumberOfOutputBands() const;

	double round(double x);

	virtual bool saveState(ossimKeywordlist& kwl,
		const char* prefix=0)const;

	/*!
	* Method to the load (recreate) the state of an object from a keyword
	* list.  Return true if ok or false on error.
	*/
	virtual bool loadState(const ossimKeywordlist& kwl,
		const char* prefix=0);


	void setScale(double scaleX, double scaleY);


	ossimRefPtr<ossimImageData> theBlankTile;
	ossimRefPtr<ossimImageData> theTile;


	void showImage(IplImage* src1, IplImage* src2); 

	// Declare class variables


	// Support functions for getting number of tiles, and 
	int GetTotalNumberTiles();
	void setGeometry(ossimRefPtr<ossimImageSource> handler);

private:

	int totalTiles;
	int tileNumber;
	double xScale;
	double yScale;
	int numberOfImageInputs;
	vector<ossimGeoPolygon> polygons; //stores the contours of the blobs found
	void markObjects(IplImage* in, IplImage* out);
	void extractBlobsAndContours(IplImage* detectionsMask, const ossimIrect& tileRect, ossimIrect* fullImageRect);
	ossimRefPtr<ossimImageGeometry> geom;
	void writeDetectionsToKmlFile();
	void writeShpFile();
	kmlWriter writer;

	template<class T>
	void CopyTileToIplImage(T dummyVariable, ossimRefPtr<ossimImageData> inputTile, IplImage *output, ossimIrect neighborhoodRect);

	template<class T>
	void CopyIplImageToTile(T dummyVariable, ossimRefPtr<ossimImageData> inputTile, IplImage *output);


	TYPE_DATA
};

#endif
