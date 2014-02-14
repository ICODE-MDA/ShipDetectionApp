/*******************************************************************************************
 * File: shapefileClip.cpp
 * 
 * Description: This code reads in an input shapefile (often the World Vector Shoreline) and crops the shapefile to fit into the image boundary.
 *
 *******************************************************************************************/

#include "shapefileClip.h"

shapefileClip::shapefileClip(ossimRefPtr<ossimImageHandler> handler, ossimFilename shpName)
	:m_handler(handler),
	m_landmaskShpName(shpName)
{
}

int shapefileClip::getClippedSHP(vector<ossimPolygon> &polys, ImageType &imageComposition)
{
	string croppedShpStr = "clippedPolygons.shp";

	// Determine if only shapefile exists and delete it.
	ifstream polygonFile(croppedShpStr.c_str());
	if (polygonFile){
		std::remove(croppedShpStr.c_str());
	}

	char* croppedFilename = (char*)croppedShpStr.c_str();

	ossimRefPtr<ossimImageGeometry> ImageGeom;
	ImageGeom = m_handler->getImageGeometry();

	// Set projection via geom info
	ossimProjection *proj = ImageGeom->getProjection();

	SHPHandle hShoreline;
	OpenShapeFile(hShoreline);
	vector<ossimPolygon> clippedPolys, holePolygons;

	ossimDrect imageBounds;
	imageBounds.setOrientMode(OSSIM_RIGHT_HANDED);
	imageBounds = LineSampleToWorld(m_handler->getBoundingRect(0), ImageGeom);

	// Find vertices within image rectangle boundary and save as a vector of ossimPolygons
	vector<int> ShapeID;
	bool allLandFlag = FindVerticesInImage(hShoreline, imageBounds, ShapeID);

	if (!ShapeID.empty()){			
		//Image contains land and water
		imageComposition = LAND_AND_WATER;

		// Clips polygons to image rectange boundary
		ClipShapesToRect(polys, holePolygons, ShapeID, hShoreline, imageBounds);
		CreateShapeFile(polys, croppedFilename);
		SHPClose(hShoreline);
	}
	else if (allLandFlag){
		//Image contains only land
		imageComposition = ALL_LAND;
		SHPClose(hShoreline);
	}
	else{
		//Image contains no land
		imageComposition = ALL_WATER;
		SHPClose(hShoreline);
	}
	return 0;
}

void shapefileClip::ClipShapesToRect(vector<ossimPolygon> &outputPolys, vector<ossimPolygon> &holePolygons, vector<int> &ShapeID, SHPHandle &hSHP, ossimDrect &imageBoundRect)
{
	if (!outputPolys.empty())
		outputPolys.clear();

	if (ShapeID.empty())
		return;

	if (hSHP == NULL)
	{
		cerr << "Shapefile is not open." << endl;
	}

	SHPObject *obj;
	ossimPolygon poly;
	vector<ossimDpt> vertices;
	vector<ossimPolygon> clippedPolys;

	ossimDpt firstVertex, lastVertex;
	int totalVert;
	//
	// Convert the shapes from SHPObject to ossimPolygon. Clip the polygons
	// to the bounds of the tile.
	//
	for (unsigned int shapeNum = 0; shapeNum < ShapeID.size(); ++shapeNum)
	{
		obj = SHPReadObject(hSHP, ShapeID[shapeNum]);
		if (!obj)
		{
			cerr << "Shape object unable to load. " << endl
			<< "Line: " << __LINE__ << " " << __FILE__ << endl;

			SHPClose(hSHP);
			exit(EXIT_FAILURE);
		}

		int partStart, partEnd;

		//
		// Shapes may have more than one part, so we need to
		// extract these polygons from the shape.
		//
		for (int part = 0; part < obj->nParts; ++part)
		{
			partStart = obj->panPartStart[part];

			if (part != obj->nParts - 1)
				partEnd = obj->panPartStart[part + 1];
			else
				partEnd = obj->nVertices;

			for (int i = partStart; i < partEnd; ++i)
				vertices.push_back(ossimDpt(obj->padfX[i], obj->padfY[i]));

			poly = vertices;
			poly.clipToRect(clippedPolys, imageBoundRect);

			if (clippedPolys.empty())
				continue;

			for (unsigned int polyNum = 0; polyNum < clippedPolys.size(); ++polyNum){
				// Check if first and last vertex match, otherwise add first vertex to polygon
				totalVert = clippedPolys[polyNum].getNumberOfVertices();
				clippedPolys[polyNum].vertex(0, firstVertex);
				clippedPolys[polyNum].vertex(totalVert-1, lastVertex);

				if (firstVertex != lastVertex){
					clippedPolys[polyNum].addPoint(firstVertex);
				}

				if (part == 0){
					outputPolys.push_back(clippedPolys[polyNum]);
				}
				else{
					holePolygons.push_back(clippedPolys[polyNum]);
				}
			}

			clippedPolys.clear();
			vertices.clear();
		}

		clippedPolys.clear();
		vertices.clear();
		SHPDestroyObject(obj);
	}
}

void shapefileClip::OpenShapeFile(SHPHandle &hSHP)
{
	if ((hSHP = SHPOpen(m_landmaskShpName.c_str(),"rb")) == NULL){
		cerr << "Shapefile " << m_landmaskShpName.c_str() << " could not be opened." << endl
		<< "Line: " << __LINE__ << " " << __FILE__ << endl;
	}
}

bool shapefileClip::FindVerticesInImage(SHPHandle hSHP, ossimDrect imgRect, vector<int> &ShapeID)
{
	if (!ShapeID.empty())
		ShapeID.clear();

	if (hSHP == NULL){
		cerr << "Shapefile is not open." << endl
		<< "Line: " << __LINE__ << " " << __FILE__ << endl;
	}

	// Get min and max values for image bounding rectangle.
	double minx, miny, maxx, maxy;
	imgRect.getBounds(minx, miny, maxx, maxy);

	ossimDrect newImgRect;
	newImgRect = ossimDrect(ossimDpt(minx, miny), ossimDpt(maxx, maxy));

	// Determine if image is completely within any of the wvs polygons
	bool allLandFlag = false;
	bool imgCompWithinPoly = false;
	bool onePointInPoly = false;

	SHPObject *obj;
	ossimDrect wvsPolyRect;
	int withinBoundsVertCount = 0;

	// Determine number of polygons in image
	unsigned long int totalPolyCount = 0;

	// loop through each shape in file
	while( obj = SHPReadObject(hSHP, totalPolyCount)){				
		totalPolyCount++;
		wvsPolyRect = ossimDrect(ossimDpt(obj->dfXMin, obj->dfYMin), ossimDpt(obj->dfXMax, obj->dfYMax));

		imgCompWithinPoly = false;
		if (newImgRect.completely_within(wvsPolyRect)){
			imgCompWithinPoly = true;		
			allLandFlag = true;
		}

		if (wvsPolyRect.intersects(newImgRect) || wvsPolyRect.completely_within(newImgRect) || imgCompWithinPoly){
			withinBoundsVertCount = 0;

			if (obj->nParts > 0 && obj->nVertices > 1){
				// loop through vertices of qualified shape
				for (unsigned long int shapeVertex = 0; shapeVertex < (unsigned)obj->nVertices && withinBoundsVertCount==0; ++shapeVertex){	
					// If vertex is within bounding box, then save
					if ((double)obj->padfX[shapeVertex] > minx &&
						(double)obj->padfX[shapeVertex] < maxx &&
						(double)obj->padfY[shapeVertex] > miny &&
						(double)obj->padfY[shapeVertex] < maxy){
							
						++withinBoundsVertCount; 
					}
				}
				
				if (withinBoundsVertCount){ 
					ShapeID.push_back(obj->nShapeId);
				}
				else if (imgCompWithinPoly){		// If bounding box completely within polygon and no vertices found
					ossimPolygon entirePoly;
					for (int j = 0; j < obj->nVertices; ++j)
						entirePoly.addPoint(ossimDpt(obj->padfX[j], obj->padfY[j]));
					
					// Check if image upper left point is within the shapefile polygon
					if (entirePoly.isPointWithin(ossimDpt(minx, miny))){
						onePointInPoly = true;
					}
				}
			}
		}
		
		// Image completely within flag
		imgCompWithinPoly = false;

		SHPDestroyObject(obj);
	}

	SHPDestroyObject(obj);
	obj = NULL;

	// If image boundary is completely within a polygon, but vertices were found within the image boundary, image contains all land
	if (allLandFlag && onePointInPoly && ShapeID.empty()){
		return true;
	}
	else{
		return false;
	}
}


ossimDrect shapefileClip::LineSampleToWorld(ossimIrect rect, ossimRefPtr<ossimImageGeometry> ImageGeom)
{
	ossimGpt gp1;
	ossimGpt gp2;
	ossimGpt gp3;
	ossimGpt gp4;

	ImageGeom->localToWorld(rect.ul(), gp1);
	ImageGeom->localToWorld(rect.ur(), gp2);
	ImageGeom->localToWorld(rect.lr(), gp3);
	ImageGeom->localToWorld(rect.ll(), gp4);
   
	ossimDrect boundsRect(ossimDpt(gp1.lond(), gp1.latd()),
		ossimDpt(gp2.lond(), gp2.latd()),
		ossimDpt(gp3.lond(), gp3.latd()),	
		ossimDpt(gp4.lond(), gp4.latd()),
		OSSIM_RIGHT_HANDED);

	return boundsRect;
}

void shapefileClip::CreateShapeFile(vector<ossimPolygon> &polys, const char *shapeFilename)
{
	if (polys.empty())
	{
		cerr << "The polygons do not exist...cannot create shape file" << endl
		<< "Line: " << __LINE__ << " " << __FILE__ << endl;
		return;
	}

	SHPObject *psObject;
	SHPHandle hSHP;
	double *px, *py;
	vector<ossimPolygon>::iterator polyIter;
	vector<ossimDpt> vertices;
	vector<ossimDpt>::iterator vertIter;

	hSHP = SHPCreate(shapeFilename, SHPT_POLYGON);

	if( hSHP == NULL || shapeFilename == NULL)
	{
		cerr << "Unable to create: " << shapeFilename <<  endl
		<< "Line: " << __LINE__ << " " << __FILE__ << endl;
		exit(EXIT_FAILURE);
	}
	// Build DBF.
	DBFHandle hDBF;

	hDBF = DBFCreate(shapeFilename);
	if( hDBF == NULL)
	{
		cerr << "Unable to create: " << shapeFilename << " DBF File" <<  endl
		<< "Line: " << __LINE__ << " " << __FILE__ << endl;
		exit(EXIT_FAILURE);
	}


	// Add fields.
	DBFAddField(hDBF, "poly_num", FTString, 30, 0);

	polyIter = polys.begin();

	int arrayIndex;
	int poly_num = 0;
	while (polyIter != polys.end())
	{
		DBFWriteStringAttribute(hDBF, poly_num, 0, ossimString::toString(poly_num));
		vertices = (*polyIter).getVertexList();
		vertIter = vertices.begin();

		double *x = new double[vertices.size()];
		double *y = new double[vertices.size()];
		int *panParts = new int[vertices.size()];
		panParts[0] = 0;

		// SHPCreateObject deletes these
		px = &x[0];
		py = &y[0];

		arrayIndex = 0;
		while (vertIter != vertices.end())
		{
			x[arrayIndex] = (*vertIter).x;
			y[arrayIndex++] = (*vertIter).y;
			vertIter++;
		}

		psObject = SHPCreateObject( SHPT_POLYGON, -1, 1, panParts, NULL,
				vertices.size(), px, py, NULL, NULL );
		if (psObject != NULL)
		{
			SHPWriteObject( hSHP, -1, psObject );
			SHPDestroyObject( psObject );
		}
		else
		{
			cerr << "Shape object could not be written to file" << endl
			<< "Line: " << __LINE__ << " " << __FILE__ << endl;
			SHPClose(hSHP);
			delete []x;
			delete []y;
			delete [] panParts;
			exit(EXIT_FAILURE);
		}

		polyIter++;
		poly_num++;
		delete []x;
		delete []y;
		delete [] panParts;
	}
	DBFClose(hDBF);
	SHPClose( hSHP );
}
