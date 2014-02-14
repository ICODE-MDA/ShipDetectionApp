//*******************************************************************
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
//
//	Main function to call ossim filter which tiles image and runs something on it
//
// 


#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/base/ossimContainerProperty.h>
#include <ossim/base/ossimDatum.h>
#include <ossim/base/ossimDatumFactoryRegistry.h>
#include <ossim/base/ossimDrect.h>
#include <ossim/base/ossimEllipsoid.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/imaging/ossimFilterResampler.h>
#include <ossim/base/ossimGeoidManager.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/init/ossimInit.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimProperty.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimXmlDocument.h>
#include <ossim/elevation/ossimElevManager.h>
#include <ossim/imaging/ossimFilterResampler.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>
#include <ossim/imaging/ossimMaskFilter.h>
#include <ossim/init/ossimInit.h>
#include <ossim/plugin/ossimSharedPluginRegistry.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/support_data/ossimInfoBase.h>
#include <ossim/support_data/ossimInfoFactoryRegistry.h>
#include <ossim/support_data/ossimSupportFilesList.h>
#include <ossim/base/ossimStdOutProgress.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimTimer.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/imaging/ossimImageRenderer.h>
#include <ossim/imaging/ossimSingleImageChain.h>
#include <ossim/imaging/ossimTiffWriter.h>
#include <vector>
#include <iostream>
#include "shipDetectionFilter.h"
#include "shapefileClip.h"

using namespace std;

int main(int argc, char *argv[])
{

	if(argc != 3){
		cout << "ossimTileToIplFilterApp.exe <input_file> <shapefile_for_mask>"<< endl;
		return 0;
	}

	cout << "In main function" << endl;

	//  Initialize ossim
	ossimInit::instance()->initialize(argc, argv);

	// the image name
	ossimFilename image_file = argv[1];
	ossimFilename inputShpName = argv[2];

	// Declare ossim ImageHandler and open input image
	ossimRefPtr<ossimImageHandler> ih = ossimImageHandlerRegistry::instance()->open(image_file);

	ImageType imageComposition;

	// Check to see if image handler is valid
	if (ih.valid())
	{
		// Instantiate the rigorous model:
		ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();

		// Now print the geometry information for the file
		cout << "Geometry: " << geom->print(cout) << endl;
		
		// Clip shapefile to image boundary
		vector<ossimPolygon> landShpPolygons;
		shapefileClip clipSHP(ih, inputShpName);
		clipSHP.getClippedSHP(landShpPolygons, imageComposition);

		//mask land
		ossimRefPtr<ossimImageHandler> inputShp = ossimImageHandlerRegistry::instance()->open(ossimFilename("clippedPolygons.shp"));
		cout << "Creating mask filter..." << endl;
		ossimRefPtr<ossimMaskFilter> maskFlt = new ossimMaskFilter();
		if ( inputShp.valid() ){
			if ( inputShp->getClassName() == "ossimOgrGdalTileSource" ){
				//The above is equivalent to saying
				//if (the input file is a shapefile)
				ossimViewInterface* shpView = PTR_CAST(ossimViewInterface, inputShp.get());
				if (shpView){
					// Test masking image handler and shape file.
					// Set the shape reader's view to that of the image's.
					shpView->setView(ih->getImageGeometry().get());
					// Turn fill on...
					ossimRefPtr<ossimProperty> fillProp =
						new ossimStringProperty(ossimString("fill_flag"),
						ossimString("1"));
					inputShp->setProperty(fillProp);

					//ossimRefPtr<ossimMaskFilter> maskFlt = new ossimMaskFilter();
					maskFlt->setMaskType(ossimMaskFilter::OSSIM_MASK_TYPE_INVERT);
					//if we wantd to mask out Water instead use the following
					//maskFlt->setMaskType(ossimMaskFilter::OSSIM_MASK_TYPE_SELECT);

					maskFlt->connectMyInputTo(0, ih.get());
					maskFlt->setMaskSource(inputShp.get());
					maskFlt->initialize();
				}
			}
		}
	
		if (imageComposition == ALL_LAND){
			cout << "Image contains only land." << endl;
			return 0;
		}

		//create ship detection filter
		cout << "Connecting handler/mask to TileToIplFilter " << endl;
		ossimRefPtr<shipDetectionFilter> shipDetection = new shipDetectionFilter();
		shipDetection->setGeometry(ih.get());

		if (imageComposition == LAND_AND_WATER){
			shipDetection->connectMyInputTo(0,maskFlt.get());
		}
		else{
			shipDetection->connectMyInputTo(ih.get());
		}

		cout << "Connected!" << endl;
		// Declare writer
		ossimRefPtr<ossimImageSourceSequencer> sequencer = new ossimImageSourceSequencer();
		sequencer->setToStartOfSequence();

		// Connect to the TileToIpl
		sequencer->connectMyInputTo(shipDetection.get());
		ossimRefPtr<ossimImageData> dataObject;
		sequencer->setTileSize(ossimIpt(512,512));

		cout << "Executing the chain..." << endl;
		// Run through image tile by tile (execute filter chain.
		while( (dataObject=sequencer->getNextTile()).valid() );

		shipDetection->disconnect();
		sequencer->disconnect();
		shipDetection = 0;
		sequencer = 0;
		//delete shipDetection ; 
		// osssim does this 
		//for us in the above code
	}
	else
		cerr << "Handler NOT valid..." << endl;

	cout << "Done!" << endl;


	return 0;

} // End of main...
