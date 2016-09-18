//-*-mode:c++; mode:font-lock;-*-

/*! \file DTED.cpp

  Classes for input and output to DTED database 

  \author     Stephen Fegan               \n
              UCLA                        \n
              sfegan@astro.ucla.edu       \n        

  \version    0.1
  \date       20/05/2005
  \note
*/

#include <fstream>
#include <string>

#include <netinet/in.h>

#include <VSDataConverter.hpp>
#include <VSDBParameterTable.hpp>
#include <VSDataConverter.hpp>

#include "DTED.hpp"

using namespace VERITAS;

// ----------------------------------------------------------------------------
// DTED Map
// ----------------------------------------------------------------------------

void DTEDMap::merge(const DTEDMap& map)
{
  assert(map.resolution() == resolution());

  std::cerr << "this: " 
	    << left() << ' ' << right() << ' ' << bottom() << ' ' << top() 
	    << std::endl; 

  std::cerr << "that: " 
	    << map.left() << ' ' << map.right() << ' ' 
	    << map.bottom() << ' ' << map.top() 
	    << std::endl; 
  
  int32_t l_this = xOf(map.left());
  int32_t r_this = xOf(map.right());
  int32_t b_this = yOf(map.bottom());
  int32_t t_this = yOf(map.top());

  std::cerr << "l_this: " << l_this << std::endl; 
  std::cerr << "r_this: " << r_this << std::endl; 
  std::cerr << "b_this: " << b_this << std::endl; 
  std::cerr << "t_this: " << t_this << std::endl; 


  if(l_this < 0)l_this=0;
  if(r_this > int32_t(width()))r_this=int32_t(width());
  if(b_this < 0)b_this=0;
  if(t_this > int32_t(height()))t_this=int32_t(height());

  std::cerr << "l_this: " << l_this << std::endl; 
  std::cerr << "r_this: " << r_this << std::endl; 
  std::cerr << "b_this: " << b_this << std::endl; 
  std::cerr << "t_this: " << t_this << std::endl; 

  if((r_this<=l_this)||(t_this<=b_this))return;

  int32_t l_that = map.xOf(xCoordOf(l_this));
  int32_t b_that = map.yOf(yCoordOf(b_this));

  std::cerr << "l_that: " << l_that << std::endl; 
  std::cerr << "b_that: " << b_that << std::endl; 

  for(unsigned y=0; y<unsigned(t_this-b_this); y++)
    {
      for(unsigned x=0; x<unsigned(r_this-l_this); x++)
	datum(x+l_this, y+b_this) = map.datum(x+l_that, y+b_that);
    }
}

DTEDMap* DTEDMap::loadMap(const std::string& filename,
			  unsigned w, unsigned h, 
			  int32_t left, int32_t bottom, 
			  uint32_t resolution)
{
  FILE* fp = fopen(filename.c_str(), "r");
  if(!fp)return 0;

  int16_t* data = new int16_t[w*h];

  for(unsigned y=0; y<h; y++)
    {
      assert(fread(data+(h-y-1)*w, sizeof(*data), w, fp) == w);
      for(unsigned x=0;x<w;x++)data[(h-y-1)*w+x] = ntohs(data[(h-y-1)*w+x]);
    }
  fclose(fp);

  return new DTEDMap(w,h,left,bottom,data,true,resolution);
}

DTEDMap* DTEDMap::loadSRTMTile(const std::string& filename,
			       uint32_t resolution)
{
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
  
  return loadMap(filename,resolution+1,resolution+1,
		 longitude*int32_t(resolution),latitude*int32_t(resolution),
		 resolution);
}

DTEDMap* DTEDMap::loadSRTMTileFromDir(const std::string& directory,
				      int32_t left, int32_t bottom,
				      uint32_t resolution)
{
  char filename[12];
  if((left<-180)||(left>179)||(bottom<-90)||(bottom>89))return 0;
  sprintf(filename,"%c%02d%c%03d.hgt",
	  (bottom<0)?'S':'N',abs(bottom),(left<0)?'W':'E',abs(left));
  std::string all_filename = std::string(filename);
  if(!directory.empty())
    all_filename = directory + std::string("/") + std::string(filename);
  std::cerr << all_filename << ' ';
  return loadMap(all_filename,resolution+1,resolution+1,
		 left*int32_t(resolution),bottom*int32_t(resolution),
		 resolution);
}

// ----------------------------------------------------------------------------
// DTED Database
// ----------------------------------------------------------------------------

DTEDDb::~DTEDDb()
{
  // nothing to see here
}

void DTEDDb::createTables()
{
  VSDBParameterTable parameter_table(fDB);
  parameter_table.createParameterTable();
  
  // stupid to create this --- C++ should have a "typeof" operator
  // (g++ does but it is not portable)
  DTEDData temp; 
  fDB->createTable(DTEDDB_DATA_TABLE,
		  fDB->sqlSpecOf("Longitude",temp.fLongitude,true,"NOT NULL")+
		  fDB->sqlSpecOf("Latitude",temp.fLatitude,false,"NOT NULL")+
		  fDB->sqlSpecOf("Elevation",temp.fElevation,false,"NOT NULL")+
		   ", PRIMARY KEY ( Longitude, Latitude )",
		   VSDatabase::FLAG_NO_ERROR_ON_EXIST_OR_NOT_EXIST);  
}

void DTEDDb::setParameters(const DTEDParameters& parameters)
{
  VSDBParameterTable parameter_table(fDB);
  VSDBParameterSet parameter_set;
  
  parameter_set["Description"] = parameters.fDescription;
  switch(parameters.fProjection)
    {
    case DTEDParameters::P_UNKNOWN:
      parameter_set["Projection"] = "Unknown";
      break;
    case DTEDParameters::P_DTED:
      parameter_set["Projection"] = "DTED";
      break;
    }  
  parameter_set["ArcMinResolution"] = 
    VSDataConverter::toString(parameters.fPointsPerDegree);
  parameter_set["VoidValue"] = 
    VSDataConverter::toString(parameters.fVoidValue);

  parameter_table.storeParameterSet(DTEDDB_PARAMTER_COLLECTION, parameter_set);
}

void DTEDDb::getParameters(DTEDParameters& parameters)
{
  VSDBParameterTable parameter_table(fDB);
  VSDBParameterSet parameter_set;

  parameter_table.retrieveParameterSet(DTEDDB_PARAMTER_COLLECTION, 
				       parameter_set);
  
  parameters.fDescription = parameter_set["Description"];
  if(parameter_set["Projection"]=="DTED")
    parameters.fProjection = DTEDParameters::P_DTED;
  else parameters.fProjection = DTEDParameters::P_UNKNOWN;
  VSDataConverter::fromString(parameters.fPointsPerDegree, 
			      parameter_set["ArcMinResolution"]);
  VSDataConverter::fromString(parameters.fVoidValue,
			      parameter_set["VoidValue"]);
}

int DTEDDb::loadMapViaFile(const DTEDMap& map, const std::string filename)
{
  getParameters(fParameters);

  std::ofstream stream(filename.c_str());
  for(unsigned y = 0; y < map.height(); y++)
    for(unsigned x = 0; x < map.width(); x++)
      if(map(x,y)!=fParameters.fVoidValue)
	{
	  fBoundData.fLongitude = map.left() + x;
	  if(fBoundData.fLongitude >= 
	     int32_t(fParameters.fPointsPerDegree*180))
	    fBoundData.fLongitude -= int32_t(fParameters.fPointsPerDegree*360);
	  fBoundData.fLatitude  = map.bottom() + y;
	  fBoundData.fElevation = map(x,y);
	  stream << fBoundData.fLongitude << '\t'
		 << fBoundData.fLatitude << '\t'
		 << fBoundData.fElevation << std::endl;
	}
  stream.close();

  VSDBStatement* stmt = 
    fDB->createQuery(std::string("LOAD DATA INFILE '")+filename+
		     std::string("' IGNORE INTO TABLE ")+
		     std::string(DTEDDB_DATA_TABLE),
		     VSDatabase::FLAG_NO_SERVER_PS);
  int c = stmt->execute();
  delete stmt;
  return c;
}

int DTEDDb::insertMap(const DTEDMap& map)
{
  if(fStmtInsert.get() == 0)
    {
      VSDBStatement* stmt;
      stmt = fDB->createInsertQuery(DTEDDB_DATA_TABLE, 3, "",
			      VSDatabase::FLAG_NO_ERROR_ON_EXIST_OR_NOT_EXIST);
      stmt->bindToParam(fBoundData.fLongitude);
      stmt->bindToParam(fBoundData.fLatitude);
      stmt->bindToParam(fBoundData.fElevation);
      fStmtInsert.reset(stmt);
      getParameters(fParameters);
   }
  
  int count=0;
  
  for(unsigned y = 0; y < map.height(); y++)
    for(unsigned x = 0; x < map.width(); x++)
      if(map(x,y)!=fParameters.fVoidValue)
	{
	  fBoundData.fLongitude = map.left() + int32_t(x);
	  if(fBoundData.fLongitude >= int32_t(map.resolution()*180))
	    fBoundData.fLongitude -= int32_t(map.resolution()*360);
	  fBoundData.fLatitude  = map.bottom() + int32_t(y);
	  fBoundData.fElevation = map(x,y);
	  if(fStmtInsert->execute() > 0)count++;
	}

  return count;
}

int DTEDDb::retrieveMap(DTEDMap& map)
{
  if(fStmtSelect.get() == 0)
    {
      VSDBStatement* stmt;
      stmt = 
	fDB->createSelectQuery(DTEDDB_DATA_TABLE, 
			       "Latitude>=? AND Latitude<? AND "
			       "Longitude>=? AND Longitude<?", "",
			     VSDatabase::FLAG_NO_ERROR_ON_EXIST_OR_NOT_EXIST);
      stmt->bindToResult(fBoundData.fLongitude);
      stmt->bindToResult(fBoundData.fLatitude);
      stmt->bindToResult(fBoundData.fElevation);
      fStmtSelect.reset(stmt);
      getParameters(fParameters);
    }

  for(unsigned y = 0; y < map.height(); y++)
    for(unsigned x = 0; x < map.width(); x++)
      map(x,y)=fParameters.fVoidValue;
  
  int count=0;

  int left   = map.left();
  int right  = map.right();
  int bottom = map.bottom();
  int top    = map.top();

  bool wrap  = false;

  fStmtSelect->bindToParam(left);
  fStmtSelect->bindToParam(right);
  fStmtSelect->bindToParam(bottom);
  fStmtSelect->bindToParam(top);

  if(right > int32_t(fParameters.fPointsPerDegree*180))
    {
      right = int32_t(fParameters.fPointsPerDegree*180);
      wrap = true;
    }

  fStmtSelect->execute();

  while(fStmtSelect->retrieveNextRow())
    {
      unsigned x = fBoundData.fLongitude-left;
      unsigned y = fBoundData.fLatitude-bottom;
      map(x,y) = fBoundData.fElevation;
      count++;
    }

  if(wrap)
    {
      int32_t start = right-left;
      left   = -int32_t(fParameters.fPointsPerDegree*180);
      right  = map.right() - int32_t(fParameters.fPointsPerDegree*360);

      fStmtSelect->execute();
      
      while(fStmtSelect->retrieveNextRow())
	{
	  unsigned x = fBoundData.fLongitude-left+start;
	  unsigned y = fBoundData.fLatitude-bottom;
	  map(x,y) = fBoundData.fElevation;
	  count++;
	}
    }

  return count;
}

