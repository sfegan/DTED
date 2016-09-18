//-*-mode:c++; mode:font-lock;-*-

/*! \file test.cpp

  Test program

  \author     Stephen Fegan               \n
              UCLA                        \n
              sfegan@astro.ucla.edu       \n        

  \version    0.1
  \date       20/05/2005
  \note
*/

#include <string>
#include <sstream>
#include <vector>
#include <cmath>

#include <DTED.hpp>

using namespace VERITAS;

int main(int argc, char** argv)
{
  const double wgs84_a = 6378136.49; // m
  const double wgs84_b = 6356751.7;
  const double wgs84_r = (wgs84_a+wgs84_b)/2.0;

  const double search_radius = 9*80; // Nine rings * 80m seperation

  const int32_t x_off[] = { 1, 1, 0, -1, -1, -1 , 0, 1 };
  const int32_t y_off[] = { 0, 1, 1, 1, 0, -1 , -1, -1 };

  char* program = *argv;
  argv++, argc--;

  while(argc)
    {
      std::string filename(*argv);
      DTEDMap* tile = DTEDMap::loadSRTMTile(filename);

      if(tile)
	{
	  std::cerr << std::endl
		    << "-----------------------------------------------------------------------------" <<std::endl
		    << "Loaded " << filename << std::endl
		    << std::endl;

	  int32_t l = tile->left();
	  int32_t r = tile->right();
	  int32_t t = tile->top();
	  int32_t b = tile->bottom();

#if 0
	  std::cerr << "l: " << l << std::endl;
	  std::cerr << "r: " << r << std::endl;
	  std::cerr << "b: " << b << std::endl;
	  std::cerr << "t: " << t << std::endl;
	  
	  std::cerr << "3*tile->resolution()+1: " 
		    << 3*tile->resolution()+1 << std::endl;
#endif

	  DTEDMap map(3*tile->resolution()+1,3*tile->resolution()+1,
		      l-tile->resolution(),b-tile->resolution(),
		      tile->resolution());

	  map.merge(*tile);
	  delete tile;
	  
	  char* last_slash = 0;
	  for(char* find = *argv; *find; find++)if(*find=='/')last_slash=find;
	  
	  std::string dir;
	  if(last_slash != 0) { *last_slash = '\0'; dir = std::string(*argv); }

	  for(unsigned i=0;i<8;i++)
	    {
	      int32_t x = DTEDMap::round(l/map.resolution()+x_off[i],1);
	      int32_t y = DTEDMap::round(b/map.resolution()+y_off[i],1);
	      std::cerr << "x: " << x << " y: " << y << ' ';
	      tile = DTEDMap::loadSRTMTileFromDir(dir,x,y);
	      if(tile)
		{
		  std::cerr << "loaded";
		  map.merge(*tile);
		  delete tile;
		}
	      std::cerr << std::endl;
	    }

	  double mean_latitude = 
	    double(t+b)/2.0/double(map.resolution())/180.0*M_PI;
	  
	  std::vector<int> dx;
	  std::vector<int> dy;

	  double scale_y = wgs84_r*M_PI/180.0/map.resolution();
	  double scale_x = scale_y*cos(mean_latitude);

	  int ny = int(ceil(search_radius/scale_y))+1;
	  int nx = int(ceil(search_radius/scale_x))+1;
	  for(int iy=-ny;iy<=ny;iy++)
	    {
	      double delta_y = double(iy)*scale_y;
	      for(int ix=-nx;ix<=nx;ix++)
		{ 
		  double delta_x = double(ix)*scale_x;
		  double delta_r=sqrt(delta_x*delta_x+delta_y*delta_y);
		  if(delta_r<=search_radius)
		    {
		      //std::cout << ix << ' ' << iy << std::endl;
		      dx.push_back(ix);
		      dy.push_back(iy);
		    }
		}
	    }

	  int32_t x0 = map.xOf(l);
	  int32_t y0 = map.yOf(b);
	  int32_t x1 = map.xOf(r);
	  int32_t y1 = map.yOf(t);

	  unsigned n = dx.size();
	  for(int32_t y=y0; y<y1; y++)
	    for(int32_t x=x0; x<x1; x++)
	      {
		int16_t el_min = -32768;
		int16_t el_max = -32768;
		int32_t el_sum = 0;
		unsigned el_cnt = 0;

		for(unsigned i=0;i<n;i++)
		  {
		    int16_t el = map.datum(x+dx[i],y+dy[i]);
		    if(el != -32768)
		      {
			if((el_min==-32768)||(el<el_min))el_min=el;
			if((el_max==-32768)||(el>el_max))el_max=el;
			el_sum+=el;
			el_cnt++;
		      }
		  }

		if((el_min>=2500)&&((el_max-el_min)<=100)&&
		   (n-el_cnt<10))
		  std::cout << map.xCoordOf(x) << ' ' 
			    << map.yCoordOf(y) << ' '
			    << el_min << ' '
			    << el_max << ' '
			    << n << ' '
			    << el_cnt << ' '
			    << double(el_sum)/double(el_cnt) << std::endl;
	      }
	}

      argv++, argc--;
    }
}
