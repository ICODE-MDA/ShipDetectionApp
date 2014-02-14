#ifndef SHAPEFILECLIP_H_
#define SHAPEFILECLIP_H_

#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/base/ossimBooleanProperty.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/imaging/ossimMaskFilter.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimPolygon.h>
#include <fstream>
#include <time.h>	
#include <shapelib/shapefil.h>

enum ImageType{
	ALL_LAND = 0,
	ALL_WATER = 1,
	LAND_AND_WATER = 3
};

	/**
	* Class that reads an input shapefile (typically a large file), finds vertices within the input image boundary, and writes out a 
	* cropped shapefile. The class will also return the image type (all land, all water, or both land and water).
	* 
	*/
	class shapefileClip
	{
	public:		
		/*
		* @param settings the settings of the detector
		* @param imageData the image data to have the landmask added to
		*/
		shapefileClip(ossimRefPtr<ossimImageHandler> handler, ossimFilename shpName);

		/**
		* Computes a cropped version of the input shapefile for vertices which are within image boundary.
		* @param polys Vector of saved shapefile polygons within image boundary.
		* @param imageComposition Saves the image content (all land, all water, or contains both land and water).
		*/
		int getClippedSHP(vector<ossimPolygon> &polys, ImageType &imageComposition);
	private:
		/**
		* Opens the input WVS shapefile.
		* @param hSHP Returns shapefile handler.
		*/
		void OpenShapeFile(SHPHandle &hSHP);

		/**
		* Reads a rectangle input in pixel values and returns a rectangle (type double) in latitude/longitude value.
		* @param hSHP Returns shapefile handler.
		* @param rect Input rectangle (typically image boundary box) in pixel value.
		* @return A rectangle in latitude/longitude values.
		*/
		static ossimDrect LineSampleToWorld(ossimIrect rect, ossimRefPtr<ossimImageGeometry> ImageGeom);

		/**
		* Writes out the cropped landmask shapefile.
		* @param polys Vector containing the detected shapefile polygons within image boundary box.
		* @param shapeFilename Output shapefile name.
		*/
		static void CreateShapeFile(vector<ossimPolygon> &polys, const char *shapeFilename);

		/**
		* Reads input shapefiles given the shape ID, clips the polygon to the image boundary, and saves them as a vector of ossimPolyon.
		* @param outputPolys Saved shapefile polygons clipped to image boundary.
		* @param holePolygons Saved shapefile holes, such as lakes, clipped to image boundary.
		* @param ShapeID Shape ID of shapefile polygons within image boundary.
		* @param hSHP Shapefile handler.
		* @param imageBoundRect Rectangle containing image boundary in latitude/longitude values.
		*/
		static void ClipShapesToRect(vector<ossimPolygon> &outputPolys, vector<ossimPolygon> &holePolygons, vector<int> &ShapeID, SHPHandle &hSHP, 
			ossimDrect &imageBoundRect);

		/**
		* Finds the vertices within image boundary. Saves the detected polygon shape ID.
		* @param hSHP Shapefile handler.
		* @param ShapeID Shape ID of shapefile polygons within image boundary.
		* @param imageBounds Rectangle containing image boundary in latitude/longitude values.
		*/
		static bool FindVerticesInImage(SHPHandle hSHP, ossimDrect imageBounds, vector<int> &ShapeID);

		string m_landmaskShpName;
		ossimRefPtr<ossimImageHandler> m_handler;
	};

#endif
