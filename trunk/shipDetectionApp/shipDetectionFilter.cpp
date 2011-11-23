/**
* OVERVIEW:
* Filter which converts each tile to IplImage .
*
* Makes use of the ossim framework and opencv libraries.
* The main operation starts with the getTile function.
* Ossim splits up the image into tiles and for each tile,
* the getTile function is called.
*
*/

#include "shipDetectionFilter.h"
#define DEBUG_FILTER 0


RTTI_DEF1(shipDetectionFilter, "shipDetectionFilter", ossimImageCombiner)

	shipDetectionFilter::shipDetectionFilter(ossimObject* owner)
	: ossimImageCombiner(owner,
	1,
	0,
	true,
	false),
	theTile(NULL),
	tileNumber(0)
{
	xScale = 0;
	yScale = 0;
	numberOfImageInputs = 1;
	geom = 0;
	writer.open("detections.kml");
}

shipDetectionFilter::~shipDetectionFilter()
{
	writeDetectionsToKmlFile();
	writeShpFile();
}

void shipDetectionFilter::initialize()
{
	ossimImageCombiner::initialize();

	if(getInput(0))
	{
		// Force an allocate on the next getTile.
		theTile = NULL;
	}

	//	   theTile = NULL;
	//
	//	   if(!isSourceEnabled())
	//	   {
	//	      return;
	//	   }
	//
	//	   theTile = ossimImageDataFactory::instance()->create(this, this);
	//	   if(theTile.valid())
	//	   {
	//	      theTile->initialize();
	//	   }

}





void shipDetectionFilter::setScale(double scaleX, double scaleY)
{
	xScale = scaleX;
	yScale = scaleY;
}

ossimScalarType shipDetectionFilter::getOutputScalarType() const
{
	if(!isSourceEnabled())
	{
		return ossimImageCombiner::getOutputScalarType();
	}

	return OSSIM_UCHAR;
}

ossim_uint32 shipDetectionFilter::getNumberOfOutputBands() const
{
	if(!isSourceEnabled())
	{
		return ossimImageCombiner::getNumberOfOutputBands();
	}
	return 1;
}

bool shipDetectionFilter::saveState(ossimKeywordlist& kwl,
	const char* prefix)const
{
	ossimImageCombiner::saveState(kwl, prefix);

	return true;
}

bool shipDetectionFilter::loadState(const ossimKeywordlist& kwl,
	const char* prefix)
{
	ossimImageCombiner::loadState(kwl, prefix);

	return true;
}

double shipDetectionFilter::round(double x)
{
	return floor(x + 0.5);
}

ossimRefPtr<ossimImageData> shipDetectionFilter::getTile(const ossimIrect& tileRect,
	ossim_uint32 resLevel)
{

	tileNumber++;
	cout << "Getting Tile: " << tileNumber << endl;

	// Check input data sources for valid and null tiles
	ossimImageSource *imageSource = PTR_CAST(ossimImageSource, getInput(0));
	ossimRefPtr<ossimImageData> imageSourceData;
	

	if (imageSource){
		imageSourceData = imageSource->getTile(tileRect, resLevel);
	}
	
	if (!isSourceEnabled()){
		return imageSourceData;
	}
	

	if (!theTile.valid())
	{
		if(getInput(0))
		{
			theTile = ossimImageDataFactory::instance()->create(this, this);
			theTile->initialize();
		}
	}
	
	if(!imageSourceData.valid()){
	cout << "imageSouceData not valid" << endl;
	}
	if(!theTile.valid()){
		cout << "theTile.valid() not valid" << endl;
	}
	if (!imageSourceData.valid() || !theTile.valid()){
		return ossimRefPtr<ossimImageData>();
	}
	
	theTile->setOrigin(tileRect.ul());
	if (theTile->getImageRectangle() != tileRect)
	{
		theTile->setImageRectangle(tileRect);
		theTile->initialize();
	}

	IplImage *input = cvCreateImage(cvSize(tileRect.width(), tileRect.height()),IPL_DEPTH_8U,3);
	IplImage *output = cvCreateImage(cvSize(tileRect.width(),tileRect.height()),IPL_DEPTH_8U,1);

	cvZero(input);
	cvZero(output);

	// If 16 or 32 bits, downsample to 8 bits
	ossimScalarType inputType = imageSourceData->getScalarType();
	if(inputType == OSSIM_UINT16 || inputType == OSSIM_USHORT11){
		CopyTileToIplImage(static_cast<ossim_uint16>(0), imageSourceData, input, tileRect);
	}else{
		CopyTileToIplImage(static_cast<ossim_uint8>(0), imageSourceData, input, tileRect);
	}

	//cvCopy(input, output);

	// Determine the actual height and width of each tile
	ossimIrect fullImageRect;
	fullImageRect = imageSource->getBoundingRect(0);

	ossim_int32 tileHeight, tileWidth, imageWidth, imageHeight;
	tileHeight = tileRect.height();
	tileWidth = tileRect.width();

	imageWidth = fullImageRect.width();
	imageHeight = fullImageRect.height();

	ossim_int32 totRows, totCols;
	totRows = (ossim_uint32)round(imageHeight / tileHeight);
	totCols = (ossim_uint32)round(imageWidth / tileWidth);

	ossimIpt upperLeftTile = tileRect.ul();

	if ((upperLeftTile.x + 1) > fullImageRect.ul().x + totCols * tileWidth)
		tileWidth = imageWidth - totCols * tileWidth;

	if ((upperLeftTile.y + 1) > fullImageRect.ul().y + totRows * tileHeight)
		tileHeight = imageHeight - totRows * tileHeight;

	//Begin Ship Detect Algorithim

	if(tileNumber == 1){
		//Write out four corners to kml
		//writeFourCornersToKml();
		ossimDpt UL(fullImageRect.ul());
		ossimDpt UR(fullImageRect.ur());
		ossimDpt LR(fullImageRect.lr());
		ossimDpt LL(fullImageRect.ll());
		ossimGpt worldul, worldur, worldlr, worldll;
		
		geom->localToWorld(UL, worldul);
		geom->localToWorld(UR, worldur);
		geom->localToWorld(LR, worldlr);
		geom->localToWorld(LL, worldll);

		writer.writePlacemark("UL","upper left corner",worldul.lat, worldul.lon, worldul.hgt);
		writer.writePlacemark("UR","upper right corner",worldur.lat, worldur.lon, worldur.hgt);
		writer.writePlacemark("LR","lower right corner",worldlr.lat, worldlr.lon, worldlr.hgt);
		writer.writePlacemark("LL","lower left corner",worldll.lat, worldll.lon, worldll.hgt);
		
	}

	//Create a mask for your detected objects
	//Detected Object == 255 ... Everything else == 0
	markObjects(input,output);

	//Extract blobs of objects and their contours
	//ossimIrect* rect = new ossimIrect(tileRect);
	extractBlobsAndContours(output, tileRect, &fullImageRect);
	//delete rect;


	// Create sub-image to ignore zeros created by OSSIM
	// ie, the tile is 512x512 but on the edges, the information is only in 512x10
	/*
	CvRect subRect = cvRect(0, 0, tileWidth, tileHeight);
	IplImage *subImg = cvCreateImage(cvSize(tileWidth, tileHeight),IPL_DEPTH_8U,3);
	cvSetImageROI(input, subRect);
	cvCopy(input, subImg);
	cvResetImageROI(input);
	*/
	if(DEBUG_FILTER){
		showImage(input, output);
	}

	cvReleaseImage(&input);
	cvReleaseImage(&output);
	//cvReleaseImage(&subImg);

	return theTile;
}



template<class T>
void shipDetectionFilter::CopyTileToIplImage(T dummyVariable, ossimRefPtr<ossimImageData> inputTile, IplImage *output, ossimIrect neighborhoodRect)
{
	ossimDataObjectStatus status = inputTile->getDataObjectStatus();

	uchar *outputData = (uchar *)output->imageData;
	int outputStep = output->widthStep/sizeof(uchar);
	int outputChannels = output->nChannels;

	ossimScalarType inputType = inputTile->getScalarType();
	double scFactor;

	if (inputType == OSSIM_UINT16)
		scFactor = 0.0039; // 255 / 65535
	else if (inputType == OSSIM_USHORT11)
		scFactor = 0.1246; //255 / 2047
	else if (inputType == OSSIM_UINT8)
		scFactor = 1;
	else
		scFactor = 1;

	int pixVal;

	if (status == OSSIM_PARTIAL)
	{
		for( int band = 0; band < outputChannels; band++){
			T* inBuf = static_cast<T*>(inputTile->getBuf(band));
			for (long y = 0; y < output->height; ++y)
			{
				for (long x = 0; x < output->width; ++x)
				{

					pixVal = (int)(*inBuf);

					if ((int)round(pixVal * scFactor) > 255)
						outputData[y * outputStep + x*outputChannels + band] = 255;
					else if ((int)round(pixVal * scFactor) < 0)
						outputData[y * outputStep + x*outputChannels + band] = 0;
					else
						outputData[y * outputStep + x*outputChannels + band] = (uchar)round(pixVal * scFactor);

					++inBuf;
				}
			}
		}
	}
	else
	{
		for(int band = 0; band < outputChannels; band++){
			T* inBuf = static_cast<T*>(inputTile->getBuf(band));
			for (int y = 0; y < output->height; ++y)
			{
				for (int x = 0; x < output->width; ++x)
				{
					pixVal = (int)(*inBuf);

					if ((int)round(pixVal * scFactor) > 255)
						outputData[y * outputStep + x*outputChannels + band] = 255;
					else if ((int)round(pixVal * scFactor) < 0)
						outputData[y * outputStep + x*outputChannels + band] = 0;
					else
						outputData[y * outputStep + x*outputChannels + band] = (uchar)round(pixVal * scFactor);

					++inBuf;
				}
			}
		}
	}
}

template<class T>
void shipDetectionFilter::CopyIplImageToTile(T dummyVariable, ossimRefPtr<ossimImageData> inputTile, IplImage *output)
{
	// Determine if tile is full or partially filled
	ossimDataObjectStatus status = inputTile->getDataObjectStatus();

	uchar *outputData = (uchar *)output->imageData;
	int outputStep = output->widthStep/sizeof(uchar);

	int pixVal;
	long outputOffset = 0;

	T maxPix = static_cast<T>(getMaxPixelValue(0));
	T minPix = static_cast<T>(getMinPixelValue(0));
	T np = static_cast<T>(inputTile->getNullPix(0));

	if (status == OSSIM_PARTIAL)
	{
		for (ossim_uint32 bandIdx = 0; bandIdx < inputTile->getNumberOfBands(); ++bandIdx)
		{
			T* outBuf = static_cast<T*>(inputTile->getBuf(bandIdx));

			if (outBuf)
			{
				outputOffset = 0;
				for (long y = 0; y < output->height; ++y)
				{
					for (long x = 0; x < output->width; ++x)
					{
						if (!inputTile->isNull(outputOffset))
						{
							pixVal = (int)round(outputData[y * outputStep + x]);

							if (pixVal > maxPix)
								*outBuf = maxPix;
							else if (pixVal < 0)
								*outBuf = minPix;
							else
								*outBuf = static_cast<T>(pixVal);
						}
						else
							inputTile->setNull(outputOffset);

						++outBuf;
						++outputOffset;
					}
				}
			}
		}
	}
	else
	{
		for (ossim_uint32 bandIdx = 0; bandIdx < inputTile->getNumberOfBands(); ++bandIdx)
		{
			T* outBuf = (T*)(inputTile->getBuf(bandIdx));

			if (outBuf)
			{
				for (int y = 0; y < output->height; ++y)
				{
					for (int x = 0; x < output->width; ++x)
					{
						pixVal = (int)round(outputData[y * outputStep + x]);

						if (pixVal > maxPix)
							*outBuf = (maxPix);
						else if (pixVal < 0)
							*outBuf = (minPix);
						else
							*outBuf = static_cast<T>(pixVal);

						// Increment the output buffer to the next pixel value
						++outBuf;
					}
				}
			}
			else
				*outBuf = np;
		}
	}
}

void shipDetectionFilter::showImage(IplImage* src1, IplImage* src2)
{
	if (src1==NULL || src2==NULL)
		cout << "ERROR ERROR _____ERROR " << endl;

	// create window
	cvNamedWindow("image1", CV_WINDOW_AUTOSIZE);
	cvMoveWindow("image1", 100, 100);
	cvNamedWindow("image2", CV_WINDOW_AUTOSIZE);
	cvMoveWindow("image2", 600, 100);

	// show the image
	cvShowImage("image1", src1);
	cvShowImage("image2", src2);

	// wait for a key
	int userKey =  cvWaitKey(0);

	if(userKey == 81 || userKey==113){ //exit if user presses q or Q
		exit(1);
	}

	cvDestroyWindow("image1");
	cvDestroyWindow("image2");

}

int shipDetectionFilter::GetTotalNumberTiles()
{
	return totalTiles;
}

void shipDetectionFilter::setGeometry(ossimRefPtr<ossimImageSource> handler){
	geom = handler->getImageGeometry();
}


void shipDetectionFilter::markObjects(IplImage* in, IplImage* out){
	//Insert your better detection here

	//create a 1 channel image for grayscale image
	//this is the same size as the input
	IplImage* grayImage =
		cvCreateImage(
		cvSize(in->width, in->height), 
		IPL_DEPTH_8U, 
		1);

	//use gray image
	//convert input image to grayscale image
	cvCvtColor(in, grayImage, CV_BGR2GRAY);
	cvThreshold(grayImage, grayImage, 100, 255, CV_THRESH_BINARY);
	//This is where you would put your
	//super awesome ship detector

	//at this point we have a binary image of our detections
	//so now we want to detect the blobs in the image
	//these blobs are our targets

	//perform a closing operation
	cvDilate(grayImage, grayImage); //local max
	cvErode(grayImage, grayImage); //local min

	//now grayImage is "closed"
	cvCopy(grayImage, out); //copy the result to the output
	cvReleaseImage(&grayImage); //free the memory
}

void shipDetectionFilter::extractBlobsAndContours(IplImage* detectionsMask, const ossimIrect& tileRect, ossimIrect* fullImageRect){

	int AREA_THRESH_LOWER = 5;

	ossimIpt iUpperLeftTilePoint = tileRect.ul();
	ossimIpt contourPoint;

	int offset = 2;	//this is the size of border we are making around the input when we create blobMask
	IplImage *blobMask = cvCreateImage(cvSize(detectionsMask->width+2*offset, detectionsMask->height+2*offset), IPL_DEPTH_8U, 1);	//add padding so blob won't touch edge
	IplImage *blobMaskCrop = cvCreateImage(cvSize(detectionsMask->width, detectionsMask->height), IPL_DEPTH_8U, 1);
	cvCopyMakeBorder(detectionsMask, blobMask, cvPoint(offset,offset), IPL_BORDER_REPLICATE);

	CBlobResult blobs = CBlobResult(blobMask, NULL, 0);

	if (blobs.GetNumBlobs() > 0)
	{
		//get rid of blobs smaller than AREA_THRESH
		blobs.Filter(blobs, B_INCLUDE, CBlobGetArea(),  B_GREATER, AREA_THRESH_LOWER); 
	}

	// mark the blobs on the image
	// delare a single blob
	CBlob Blob;
	ossimIrect blobRect;
	static int currentBlobNumber = 1;
	int numBlobs = blobs.GetNumBlobs();

	for  (int i=0; i<numBlobs; ++i)
	{
		// get the blob info
		Blob = blobs.GetBlob(i);
		//cout << "blob(" << i << ").Area()=" <<Blob.Area() << endl;

		//GET CONTOUR FOR KML POLYGON
		cvSetZero(blobMask);
		CvScalar color = CV_RGB(255,255,255);
		Blob.FillBlob(blobMask,color);		//set all pixels in blob mask =255
		if(DEBUG_FILTER){
			cvShowImage("blob", blobMask);
			cvWaitKey(0);
		}
		CvMemStorage* storage = cvCreateMemStorage(0);
		CvSeq* contours = NULL;
		cvFindContours(blobMask, storage, &contours, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
		int contourCount = 0;
		
		for(CvSeq* currentContour = contours; currentContour!=NULL; currentContour=currentContour->h_next)
		{
			contourCount++;
			vector<ossimGpt> contour;
			for(int contourPosition = 0; contourPosition < currentContour->total; ++contourPosition)
			{
				CvPoint* p = CV_GET_SEQ_ELEM(CvPoint, currentContour, contourPosition);
				/*cout << p->x << " " << p->y << endl;
				cout << geom->getMetersPerPixel() << endl;
				ossimGpt tempGpt;
				geom->localToWorld(ossimDpt(p->x,p->y),tempGpt);
				cout << "lat: " << tempGpt.lat << 
					" lon:" << tempGpt.lon << endl;*/
				contourPoint = ossimIpt(p->x-offset, p->y-offset) + iUpperLeftTilePoint;
				ossimDpt temp_dpt = ossimDpt(contourPoint.x, contourPoint.y);
				ossimGpt temp_gpt;
				geom->localToWorld(temp_dpt, temp_gpt);
				contour.push_back(temp_gpt);
				//writer.writePlacemark("contour points","",temp_gpt.lat, temp_gpt.lon, 0);
			}
			polygons.push_back(contour);
			contour.clear();
		}
		currentBlobNumber++;
		cvReleaseMemStorage(&storage);
		//END OF GET CONTOUR FOR KML POLYGON
	}
	cvReleaseImage(&blobMask);
	cvReleaseImage(&blobMaskCrop);
	blobs.ClearBlobs();
}


void shipDetectionFilter::writeDetectionsToKmlFile(){
	for(int i = 0; i < polygons.size(); i++){
		string name = "Ship ";
		char index[10];
		itoa(i, index, 10);
		name+=index;
		writer.writePolygon(name, " ", polygons[i], 1, 1, "clampToGround");
	}
}


void shipDetectionFilter::writeShpFile(){
	ossimFilename shapefileName("detections.shp");
	SHPHandle hSHP = SHPCreate(shapefileName.c_str(),SHPT_POLYGON);

	SHPObject *psObject;
	if(hSHP == NULL){
		cout << "Shapefile could not be created" << endl;
	}

	DBFHandle hDBF;
	hDBF = DBFCreate(shapefileName.c_str());
	
	//Add all fields that we wish to access from DBF
	DBFAddField(hDBF, "Area", FTString, 30, 0);
	/*DBFAddField(hDBF, "Length", FTString, 30, 0);
	DBFAddField(hDBF, "Width", FTString, 30, 0);*/

	for(int i = 0; i<polygons.size(); i++){
		int nVertices = polygons[i].size();
		
		int* panParts = new int[nVertices+1];
		double* padfX = new double[nVertices+1];
		double* padfY = new double[nVertices+1];
		
		for(int j=0; j<nVertices; j++){
			padfX[j] = polygons[i][j].lon;
			padfY[j] = polygons[i][j].lat;
			//padfZ[j] = polygons[i][j].hgt;
		}
		//the last must be equal to the first
		padfX[nVertices] = polygons[i][0].lon;
		padfY[nVertices] = polygons[i][0].lat;
		
		psObject = SHPCreateObject(SHPT_POLYGON, -1, 1, panParts, NULL, nVertices + 1,
				padfX, padfY, NULL, NULL);
		SHPWriteObject(hSHP, -1, psObject );
		DBFWriteStringAttribute(hDBF, i, 0, 
			ossimString::toString(polygons[i].area()));
		/*
		//for length
		DBFWriteStringAttribute(hDBF, i, 1, 
			ossimString::toString(123));
		//for width
		DBFWriteStringAttribute(hDBF, i, 2, 
			ossimString::toString(456));*/
		SHPDestroyObject(psObject );
		delete [] panParts;
		delete [] padfX;
		delete [] padfY;
	}
	
	SHPClose(hSHP );
	DBFClose( hDBF );
}