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
  const uint32_t TILERES = 1200;

  const double wgs84_a = 6378136.49; // m
  const double wgs84_b = 6356751.7;
  const double wgs84_r = (wgs84_a+wgs84_b)/2.0;

  std::string directory;

  double lat_zero = 34.05;          // Degrees (default is centered on LA)
  double long_zero = -118.25;       // Degrees (default is centered on LA)
  double radius = 10;               // KM
  double approx_resolution = 0.090; // KM
  
  char* progname = *argv;
  argv++, argc--;

  if(argc == 0)
    {
      std::cerr << "Usage: " << progname 
		<< " directory [long] [lat] [radius] [res]" << std::endl;
      exit(EXIT_FAILURE);
    }

  directory = std::string(*argv);
  argv++, argc--;

  if(argc)
    {
      std::istringstream stream(*argv);
      stream >> long_zero;
      argv++, argc--;
    }

  if(argc)
    {
      std::istringstream stream(*argv);
      stream >> lat_zero;
      argv++, argc--;
    }

  if(argc)
    {
      std::istringstream stream(*argv);
      stream >> radius;
      argv++, argc--;
    }

  if(argc)
    {
      std::istringstream stream(*argv);
      stream >> approx_resolution;
      argv++, argc--;
    }

  radius *= 1000;
  approx_resolution *= 1000;

  int32_t cen_x = int32_t(floor((long_zero)*double(TILERES)));
  int32_t cen_y = int32_t(floor((lat_zero)*double(TILERES)));

  std::cerr << "Center:   " << long_zero << ',' << lat_zero 
	    << " (" << cen_x << ',' << cen_y << ')' << std::endl;

  double v_extent = 
    ceil(radius/approx_resolution+1)*approx_resolution/wgs84_r/M_PI*180.0;
  double h_extent = 
    ceil(radius/cos(lat_zero/180.0*M_PI)/approx_resolution+1)
    *approx_resolution/wgs84_r/M_PI*180.0;

  std::cerr << "Extent:   " << h_extent << " x " << v_extent 
	    << " deg" << std::endl;
  
  int32_t bound_l = int32_t(floor((long_zero - h_extent)*double(TILERES)));
  int32_t bound_r = int32_t(ceil((long_zero + h_extent)*double(TILERES)));
  int32_t bound_b = int32_t(floor((lat_zero - v_extent)*double(TILERES)));
  int32_t bound_t = int32_t(ceil((lat_zero + v_extent)*double(TILERES)));

  std::cerr << "Boundary: " 
	    << bound_l << ',' << bound_b << " -> " 
	    << bound_r << ',' << bound_t << std::endl;

  int32_t tile_l = bound_l/TILERES;
  if(bound_l<0)tile_l=-((abs(bound_l)+(TILERES-1))/TILERES);
  int32_t tile_r = (bound_r+(TILERES-1))/TILERES;
  if(bound_r<0)tile_r=-(abs(bound_r)/TILERES);
  int32_t tile_b = bound_b/TILERES;
  if(bound_b<0)tile_b=-((abs(bound_b)+(TILERES-1))/TILERES);
  int32_t tile_t = (bound_t+(TILERES-1))/TILERES;
  if(bound_t<0)tile_t=-(abs(bound_t)/TILERES);

  unsigned tile_w = tile_r-tile_l;
  unsigned tile_h = tile_t-tile_b;

  std::cerr << "Tiles:    " 
	    << DTEDMap::round(tile_l,1) << ',' << tile_b << " -> " 
	    << DTEDMap::round(tile_r,1) << ',' << tile_t << " = " 
	    << tile_w << " x " << tile_h << std::endl;

 
  DTEDMap map(tile_w*TILERES+1, tile_h*TILERES+1,
	      tile_l*TILERES, tile_b*TILERES, TILERES);

  for(int32_t x = tile_l; x<tile_r; x++)
    for(int32_t y = tile_b; y<tile_t; y++)
      {
	int32_t _x = DTEDMap::round(x,1);
	int32_t _y = y;

	DTEDMap* tile = DTEDMap::loadSRTMTileFromDir(directory,_x,_y);
	if(tile)
	  {
	    std::cerr << "Loaded tile: " << _x << ',' << _y;
	    map.merge(*tile);
	    delete tile;	    
	  }
      }

  unsigned y_step = unsigned(floor(approx_resolution/wgs84_r/M_PI*180*1200));
  if(y_step==0)y_step=1;

  unsigned x_step = unsigned(floor(approx_resolution/wgs84_r/M_PI*180*1200/
				   cos(lat_zero/180.0*M_PI)));
  if(x_step==0)x_step=1;

  int32_t readout_l = (bound_l/x_step)*x_step;
  if(bound_l<0)readout_l = -((abs(bound_l)+(x_step-1))/x_step)*x_step;
  int32_t readout_r = ((bound_r+(x_step-1))/x_step)*x_step;
  if(bound_r<0)readout_r = -(abs(bound_r)/x_step)*x_step;

  int32_t readout_b = (bound_b/y_step)*y_step;
  if(bound_b<0)readout_b = -((abs(bound_b)+(y_step-1))/y_step)*y_step;
  int32_t readout_t = ((bound_t+(y_step-1))/y_step)*y_step;
  if(bound_t<0)readout_t = -(abs(bound_t)/y_step)*y_step;

  std::cerr << "Image:     " 
	    << (readout_r-readout_l)/x_step+1 << " x "
	    << (readout_t-readout_b)/y_step+1 << std::endl;

  const double scale_x = 
    1.0/double(TILERES)/180.0*M_PI*wgs84_r/1000.0*cos(lat_zero/180.0*M_PI);
  const double scale_y = 
    1.0/double(TILERES)/180.0*M_PI*wgs84_r/1000.0;

  for(int32_t x=readout_l; x<=readout_r; x+=x_step)
    for(int32_t y=readout_b; y<=readout_t; y+=y_step)
      {
	double x_deg = double(map.round(x))/double(TILERES);
	double y_deg = double(y)/double(TILERES);
	double x_fp = double(map.round(x-cen_x))*scale_x;
	double y_fp = double(y-cen_y)*scale_y;
	
	int32_t x_start = x-x_step/2;
	int32_t x_end   = x+x_step/2;
	int32_t y_start = y-y_step/2;
	int32_t y_end   = y+y_step/2;
	
	double el_avg_sum = 0.0;
	double el_avg_nrm = 0.0;
	for(int32_t ix=x_start; ix<=x_end; ix++)
	  {
	    double x_weight=1.0;
	    if((x_step%2==0)&&((ix==x_start)||(ix==x_end)))x_weight=0.5;
	    for(int32_t iy=y_start; iy<=y_end; iy++)
	      {
		double y_weight=1.0;
		if((y_step%2==0)&&((iy==y_start)||(iy==y_end)))y_weight=0.5;
		int16_t el = map(map.xOf(ix),map.yOf(iy));
		if(el != -32768)
		  {
		    el_avg_sum += double(el)*x_weight*y_weight;
		    el_avg_nrm += x_weight*y_weight;
		  }
	      }

	  }
#if 0
	std::cerr << x_start << ' ' << x_end << ' ' 
		  << y_start << ' ' << y_end << ' '
		  << el_avg_sum << ' ' << el_avg_nrm << std::endl;
#endif	

	if(el_avg_nrm)
	  std::cout << x_deg << ' ' 
		    << y_deg << ' '
		    << x_fp << ' ' 
		    << y_fp << ' '
		    << el_avg_sum/el_avg_nrm
		    << std::endl;
	else
	  std::cout << x_deg << ' ' 
		    << y_deg << ' '
		    << x_fp << ' ' 
		    << y_fp << ' '
		    << -32768
		    << std::endl;
	//	std::cout << x-ceny
      }

  return EXIT_SUCCESS;
}
