#include "kmlWriter.h"

	kmlWriter::kmlWriter(){
	}

	void kmlWriter::open(string filename){
		_filename = filename;
		_stream.open(_filename.c_str());
		if(isGood()){
			writeHeader();
		}
	}

	bool kmlWriter::isGood(){
		return _stream.good();
	}

	kmlWriter::~kmlWriter(){
		if(_stream.is_open()){
			writeFooter();
			_stream.close();
		}
	}


	void kmlWriter::writePlacemark(string name, string description, double lat, double lon, double height){
		_stream << "	<Placemark>" << endl;
		_stream << "		<name>" << name << "</name>" << endl;
		_stream << "		<description>" << description << "</description>" <<endl;
		_stream << "		<styleUrl>#msn_ylw-pushpin</styleUrl>" << endl;
		_stream << "		<Point>" << endl;
		_stream << "			<coordinates>"<< setprecision(15) << lon << "," << lat << "," << height << "</coordinates>" << endl;
		_stream << "		</Point>" << endl;
		_stream << "	</Placemark>" << endl;
	}

	void kmlWriter::writePath(string name, string description, vector< vector<double> >& coordinates, int extrude, int tessellate, string altitude){
		_stream << "	<Placemark>" << endl;
		_stream << "		<name>" << name << "</name>" << endl;
		_stream << "		<description>" << description << "</description>" << endl;
		_stream << "		<LineString>" << endl;
		_stream << "			<extrude>" << extrude << "</extrude>" << endl;
		_stream << "			<tessellate>" << tessellate << "</tessellate>" << endl;
		_stream << "			<altitudeMode>" << altitude << "</altitudeMode>" << endl;
		_stream << "			<coordinates>" << endl;
		for(int i = 0; i< coordinates.size(); i++){
			if(coordinates[i].size() == 3){
			_stream << "				" << setprecision(15)<< coordinates[i][0] << "," << coordinates[i][1] << "," <<coordinates[i][2] << endl;
			}else{
				cout << "kmlWriter::writePath coordinates must be size Nx3" << endl;
			}
		}
		_stream << "			</coordinates>" << endl;
		_stream << "		</LineString>" << endl;
		_stream << "	</Placemark>" << endl;

	}


	void kmlWriter::writePolygon(string name, string description, ossimGeoPolygon& coordinates, int extrude, int tessellate, string altitude){
		_stream << "	<Placemark>" << endl;
		_stream << "		<name>" << name << "</name>" << endl;
		_stream << "		<description>" << description << "</description>" << endl;
		_stream << "		<Polygon>" << endl;
		_stream << "			<extrude>" << extrude << "</extrude>" << endl;
		_stream << "			<tessellate>" << tessellate << "</tessellate>" << endl;
		_stream << "			<altitudeMode>" << altitude << "</altitudeMode>" << endl;
		_stream << "			<outerBoundaryIs>" << endl;
		_stream << "				<LinearRing>" << endl;
		_stream << "					<coordinates>" << endl;
		//cout << "NEED TO ADD A FOR LOOP TO WRITE EACH COORDINATE IN THE writePolygon FUNCTION OF KMLWRITER" << endl;
		//hints: use setprecision(15)
		//the coordinates need to be lon, lat, 0
		for(int i = 0; i< coordinates.size(); i++){
			_stream << "					"<< setprecision(15) 
				<< coordinates[i].lon << ","     <<
				coordinates[i].lat << ",0" <<     endl;
		}
		_stream << "					</coordinates>" << endl;
		_stream << "				</LinearRing>" << endl;
		_stream << "			</outerBoundaryIs>" << endl;
        /*
		_stream << "			<innerBoundaryIs>" << endl;
		_stream << "				<LinearRing>" << endl;
		_stream << "					<coordinates>" << endl;
		
		_stream << "					</coordinates>" << endl;
		_stream << "				</LinearRing>" << endl;
		_stream << "			</innerBoundaryIs>" << endl;
		*/
        _stream << "		</Polygon>" << endl;
		_stream << "	</Placemark>" << endl;
	}

	void kmlWriter::writeHeader(){
		_stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
		_stream << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">" << endl;
		_stream << "<Document>" << endl;
		_stream << "<Style id=\"sn_ylw-pushpin\">" << endl;
		_stream << "	<IconStyle>" << endl;
		_stream << "		<scale>1.1</scale>" << endl;
		_stream << "		<Icon>" << endl;
		_stream << "			<href>http://maps.google.com/mapfiles/kml/pushpin/ylw-pushpin.png</href>" << endl;
		_stream << "		</Icon>" << endl;
		_stream << "		<hotSpot x=\"20\" y=\"2\" xunits=\"pixels\" yunits=\"pixels\"/>" << endl;
		_stream << "	</IconStyle>" << endl;
		_stream << "</Style>" << endl;
		_stream << "<Style id=\"sh_ylw-pushpin\">" << endl;
		_stream << "	<IconStyle>" << endl;
		_stream << "		<scale>1.3</scale>" << endl;
		_stream << "		<Icon>" << endl;
		_stream << "			<href>http://maps.google.com/mapfiles/kml/pushpin/ylw-pushpin.png</href>" << endl;
		_stream << "		</Icon>" << endl;
		_stream << "		<hotSpot x=\"20\" y=\"2\" xunits=\"pixels\" yunits=\"pixels\"/>" << endl;
		_stream << "	</IconStyle>" << endl;
		_stream << "</Style>" << endl;
		_stream << "<StyleMap id=\"msn_ylw-pushpin\">" << endl;
		_stream << "	<Pair>" << endl;
		_stream << "		<key>normal</key>" << endl;
		_stream << "		<styleUrl>#sn_ylw-pushpin</styleUrl>" << endl;
		_stream << "	</Pair>" << endl;
		_stream << "	<Pair>" << endl;
		_stream << "		<key>highlight</key>" << endl;
		_stream << "		<styleUrl>#sh_ylw-pushpin</styleUrl>" << endl;
		_stream << "	</Pair>" << endl;
		_stream << "</StyleMap>" << endl;
	}

	void kmlWriter::writeFooter(){
		_stream << "</Document>" << endl;
		_stream << "</kml>" << endl;
	}

