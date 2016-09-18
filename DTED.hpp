//-*-mode:c++; mode:font-lock;-*-

/*! \file DTED.hpp

  Classes for input and output to DTED database 

  \author     Stephen Fegan               \n
              UCLA                        \n
              sfegan@astro.ucla.edu       \n        

  \version    0.1
  \date       20/05/2005
  \note
*/

#ifndef DTED_HPP
#define DTED_HPP

#include <string>
#include <memory>
#include <stdint.h>

#include <VSDatabase.hpp>

//! VERITAS namespace
namespace VERITAS 
{

  class DTEDData
  {
  public:
    int32_t     fLatitude;
    int32_t     fLongitude;
    int16_t     fElevation;
  };

  class DTEDParameters
  {
  public:
    enum Projection { P_UNKNOWN, P_DTED };
    std::string fDescription;
    Projection  fProjection;
    int32_t     fPointsPerDegree;
    int16_t     fVoidValue;
  };

  class DTEDMap
  {
  public:
    DTEDMap(unsigned w, unsigned h, int32_t left, int32_t bottom, 
	    uint32_t resolution = 1200, int16_t zero_val = -32768)
      : fResolution(resolution),
	fData(new int16_t[w*h]), fMine(true), fWidth(w), fHeight(h),
	fLeft(round(left)), fBottom(round(bottom))
    { 
      for(unsigned i=0;i<w*h;i++)fData[i]=zero_val;
    }
    DTEDMap(unsigned w, unsigned h, int32_t left, int32_t bottom,
	    int16_t* data, bool mine=false, uint32_t resolution = 1200)
      : fResolution(resolution),
	fData(data), fMine(mine), fWidth(w), fHeight(h),
	fLeft(round(left)), fBottom(round(bottom))  { }
    ~DTEDMap() { if(fMine)delete[] fData; }

    unsigned width() const { return fWidth; }
    unsigned height() const { return fHeight; }

    int32_t left() const { return fLeft; }
    int32_t right() const { return fLeft+fWidth; }
    int32_t bottom() const { return fBottom; }
    int32_t top() const { return fBottom+fHeight; }

    int16_t* data() { return fData; }
    const int16_t* data() const { return fData; }

    int16_t& datum(unsigned x, unsigned y) 
    { assert((x<fWidth)&&(y<fHeight)); return fData[y*fWidth+x]; }
    const int16_t& datum(unsigned x, unsigned y) const 
    { assert((x<fWidth)&&(y<fHeight)); return fData[y*fWidth+x]; }

    int16_t& operator() (unsigned x, unsigned y) 
    { return datum(x,y); }
    const int16_t& operator() (unsigned x, unsigned y) const 
    { return datum(x,y); }

    uint32_t resolution() const { return fResolution; }

    static int32_t round(int32_t x, uint32_t resolution)
    {
      int32_t wrap = 360 * int32_t(resolution);
      x = ((x%wrap)+wrap)%wrap;
      if(x>=wrap/2)x-=wrap;
      return x;
    }

    int32_t round(int32_t x) const { return round(x,fResolution); }
      
    int32_t xOf(int32_t x) const { return round(x-left()-width()/2)+width()/2;}
    int32_t yOf(int32_t y) const { return y-bottom(); }

    int32_t xCoordOf(int32_t x) const { return round(x+left()); }
    int32_t yCoordOf(int32_t y) const { return y+bottom(); }

    void merge(const DTEDMap& map);

    static DTEDMap* loadMap(const std::string& filename,
			    unsigned w, unsigned h, 
			    int32_t left, int32_t bottom, 
			    uint32_t resolution = 1200);
    static DTEDMap* loadSRTMTile(const std::string& filename,
				 uint32_t resolution = 1200);
    static DTEDMap* loadSRTMTileFromDir(const std::string& directory,
					int32_t left, int32_t bottom,
					uint32_t resolution = 1200);
    
  private:
    uint32_t     fResolution;
    int16_t*     fData;
    bool         fMine;
    unsigned     fWidth;
    unsigned     fHeight;
    int32_t      fLeft;
    int32_t      fBottom;
  };

#define DTEDDB_PARAMTER_COLLECTION "DTED"
#define DTEDDB_DATA_TABLE          "Elevation"

  class DTEDDb
  {
  public:
    DTEDDb(VSDatabase* db): 
      fDB(db), fStmtInsert(), fStmtSelect(), fBoundData(), fParameters() { }
    virtual ~DTEDDb();

    void createTables();
    void setParameters(const DTEDParameters& parameters);
    void getParameters(DTEDParameters& parameters);
    
    int loadMapViaFile(const DTEDMap& map,
		       const std::string filename="/tmp/dted.dat");
    int insertMap(const DTEDMap& map);
    int retrieveMap(DTEDMap& map);

  private:
    VSDatabase*                  fDB;
    std::auto_ptr<VSDBStatement> fStmtInsert; 
    std::auto_ptr<VSDBStatement> fStmtSelect; 
    DTEDData                     fBoundData;
    DTEDParameters               fParameters;
  };

}

#endif // DTED_HPP
