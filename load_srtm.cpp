//-*-mode:c++; mode:font-lock;-*-

/*! \file load_srtm.cpp

  Program to load Shuttle Radar Topograph Mission DTED-3 data

  \author     Stephen Fegan               \n
              UCLA                        \n
              sfegan@astro.ucla.edu       \n        

  \version    0.1
  \date       20/05/2005
  \note
*/

#include <string>
#include <sstream>

#include <netinet/in.h>

#include <VSOptions.hpp>
#include <VSDBFactory.hpp>
#include <DTED.hpp>

using namespace VERITAS;

int main(int argc, char ** argv)
{
  VSOptions options(argc,argv);
  VSDBFactory::configure(&options);

  // --------------------------------------------------------------------------
  // Get create database option now
  // --------------------------------------------------------------------------
  
  bool create_db = false;
  if(options.find("create_db") != VSOptions::FS_NOT_FOUND)create_db=true;

  char *progname = *argv;
  argv++, argc--;

  // --------------------------------------------------------------------------
  // Get the filename and database name from the command line arguments
  // --------------------------------------------------------------------------

  if(argc<1)
    {
      std::cerr << "Usage: " << progname 
		<< " [-create_db] database [filenames]" 
		<< std::endl;
      exit(EXIT_FAILURE);
    }

  std::string database(*argv);
  argc--; argv++;

  // --------------------------------------------------------------------------
  // Create the database if requested and connect
  // --------------------------------------------------------------------------
  
  VSDatabase* db = VSDBFactory::getInstance()->createVSDB();

  if(create_db)
    db->createDatabase(database,
		       VSDatabase::FLAG_NO_ERROR_ON_EXIST_OR_NOT_EXIST);

  db->useDatabase(database);
  DTEDDb* dted = new DTEDDb(db);

  // --------------------------------------------------------------------------
  // Create database structure
  // --------------------------------------------------------------------------

  DTEDParameters parameters;
  parameters.fDescription      = "SRTM-3 DTED";
  parameters.fProjection       = DTEDParameters::P_DTED;
  parameters.fPointsPerDegree  = 1200;
  parameters.fVoidValue        = -32768;
  
  if(create_db)
    {
      dted->createTables();
      dted->setParameters(parameters);
    };

  // --------------------------------------------------------------------------
  // Loop over files
  // --------------------------------------------------------------------------

  while(argc)
    {
      unsigned w = 1201;
      unsigned h = 1201;

      std::string filename = std::string(*argv);
      argc--; argv++;

      std::cerr << filename << ": ";

      std::string basename;
      if(filename.rfind("/") != std::string::npos)
	basename = filename.substr(filename.rfind("/")+1);
      else
	basename = filename;

      int32_t latitude = 0;
      int32_t longitude = 0;
      VSDataConverter::fromString(latitude, basename.substr(1,2));
      if(basename[0]=='S')latitude = -latitude;
      VSDataConverter::fromString(longitude, basename.substr(4,3));
      if(basename[3]=='W')longitude = -longitude;

      FILE* fp = fopen(filename.c_str(), "r");
      if(!fp)continue;

      std::cerr << longitude << ',' << latitude << ' ';

      latitude *= parameters.fPointsPerDegree; 
      longitude *= parameters.fPointsPerDegree;

      int16_t* data = new int16_t[w*h];
      assert(fread(data, sizeof(*data), w*h, fp) == w*h);
      fclose(fp);

      DTEDMap map_i(w,h,longitude,latitude,data);

      w=1200; // Last row and column is duplicated in each tile
      h=1200;
      DTEDMap map_o(w,h,longitude,latitude);

      for(unsigned y=0;y<h;y++)
	for(unsigned x=0;x<w;x++)
	  map_o(x,y) = ntohs(map_i(x,map_i.height()-y-1));

#if 0
      for(int y=0;y<h;y++)
	for(int x=0;x<w;x++)
	  if(map_o(x,y)!=parameters.fVoidValue)
	    std::cout << map_o.left()+x << '\t'
		      << map_o.bottom()+y << '\t'
		      << map_o(x,y) << std::endl;
      std::cerr << std::endl;
#endif

#if 0
      int count = dted->insertMap(map_o);
      std::cerr << count << std::endl;
#endif 

#if 1
      int count = dted->loadMapViaFile(map_o);
      std::cerr << count << std::endl;
#endif 

      delete[] data;
    }

  delete dted;
  delete db;
}
