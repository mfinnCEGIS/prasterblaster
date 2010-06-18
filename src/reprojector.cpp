
#include <cstdio>

#include <mpi.h>
#include <gdal.h>
#include <gdal_priv.h>

#include "pRPL/prProcess.h"
#include "pRPL/neighborhood.h"
#include "pRPL/cellSpace.h"
#include "pRPL/layer.h"
#include "pRPL/basicTypes.h"

#include "reproject_transition.hh"

#include "gctp_cpp/projection.h"
#include "gctp_cpp/transformer.h"
#include "gctp_cpp/coordinate.h"

#include "reprojector.hh"
#include "resampler.hh"

using namespace pRPL;

Reprojector::Reprojector(PRProcess _prc, ProjectedRaster *_input, ProjectedRaster *_output) 
	:
	prc(_prc), input(_input), output(_output), input_layer(_prc), output_layer(_prc)
{
	vector<CellCoord> coords;


	maxx = maxy = 0;
	minx = miny = 1e+37;
	resampler = &resampler::nearest_neighbor<unsigned char>;


	return;
};

Reprojector::~Reprojector()
{


return;
}

void Reprojector::parallelReproject()
{
	Transformer t;
	Coordinate temp, temp2;
	double in_ulx, in_uly, out_ulx, out_uly;
	double in_pixsize, out_pixsize;
	long in_rows, in_cols, out_rows, out_cols, out_chunk;
	vector<char> ot;

	out_chunk = 2;

	t.setInput(*output->getProjection());
	t.setOutput(*input->getProjection());
  
	in_rows = input->getRowCount();
	in_cols = input->getColCount();
	out_rows = output->getRowCount()/prc.nPrcs();// + (output->getRowCount() % prc.nPrcs());
	out_cols = output->getColCount();
	in_pixsize = input->getPixelSize();
	out_pixsize = input->getPixelSize();
	in_ulx = input->ul_x;
	in_uly = input->ul_y;
	out_ulx = output->ul_x;
	out_uly = output->ul_y - (prc.id() * ((out_rows * out_pixsize)/prc.nPrcs()));

/*
 *
 *
 *
 // CHANGE THAT 2 TO SOMETHING ELSE!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
 *
 *
 *
 */

	if (prc.id() == 0)
		return;
	
//	for (int i = 0; i < 2; ++i) {
//		reprojectChunk(90,15*i);
//	}

/*
	for (int i = out_rows * prc.id(); i < out_rows + (prc.id()*out_rows); 
	     i += out_chunk) {
		printf("id: %d::::Reprojecting from %d up to %d\n", 
		       prc.id(),
		       i, i + out_chunk);
		reprojectChunk(i, out_chunk+1);
	}
*/

	reprojectChunk(300, 20);
/*
	if (prc.id() == 1) {
		reprojectChunk(out_rows + (prc.nPrcs()*out_rows),
			       output->getRowCount() - out_rows +
			       (prc.nPrcs()*out_rows));
	}
*/

	return;
}

void Reprojector::reprojectChunk(int firstRow, int numRows)
{
	Projection *outproj, *inproj;
	Coordinate temp1, temp2;
	Coordinate in_ul, in_lr, out_ul;
	double in_pixsize, out_pixsize;
	long in_rows, in_cols, out_rows, out_cols, out_chunk;
	vector<char> inraster, outraster;

	if (firstRow + numRows > output->getRowCount()) {
		fprintf(stderr, "Invalid chunk range...\n");
		fflush(stderr);
		return;
	}

	out_pixsize = output->getPixelSize();
	in_pixsize = input->getPixelSize();
	outproj = output->getProjection();
	inproj = input->getProjection();

	out_rows = numRows;
	out_cols = output->getColCount();
	out_ul.x = output->ul_x;
	out_ul.y = output->ul_y - firstRow * out_pixsize;
	in_cols = input->getColCount();
	
	FindMinBox2(out_ul.x, out_ul.y, out_pixsize,
		    out_rows, out_cols,
		    outproj, inproj,
		    in_pixsize,
		    in_ul.x, in_ul.y, 
		    in_lr.x, in_lr.y);


	in_ul.x = input->ul_x;
	if (in_ul.y > input->ul_y)
		in_ul.y = input->ul_y;

	in_lr.x = input->ul_x + (in_pixsize * in_cols);

	in_rows = (long)((in_ul.y - in_lr.y) / in_pixsize);
	in_rows += 5;

	// Setup raster vectors
	size_t s = out_rows;
	s *= out_cols;
	s *= output->bitsPerPixel()/8;
	outraster.resize(s);
	s = in_rows;
	s *= in_cols;
	s *= output->bitsPerPixel()/8;
	inraster.resize(s);
	
	// Read input file
	if (input->readRaster(firstRow, numRows, &(inraster[0]))) {
		printf("Read %d rows\n", numRows);
	} else {
		printf("Error Reading input!\n");
	}

	for (int y = 0; y < out_rows; ++y) {
		for (int x = 0; x < out_cols; ++x) {

			temp1.x = ((double)x * out_pixsize) + out_ul.x;
//			temp1.y = ((double)y * out_pixsize) - out_ul.y;
			temp1.y = out_ul.y - ((double)y * out_pixsize);

			outproj->inverse(temp1.x, temp1.y, &temp2.x, &temp2.y);
			outproj->forward(temp2.x, temp2.y,
					 &temp2.x, &temp2.y);
			if (abs(temp1.x - temp2.x) > 0.0001) {
				continue;
			}

			temp1.x = out_ul.x + ((double)x * out_pixsize);
//			temp1.y = out_ul.y - ((double)y * out_pixsize);
			temp1.y = ((double)y * out_pixsize) - out_ul.y;


//			printf("OUTPUT COORDS: %f %f\n", temp1.x, temp1.y);
			outproj->inverse(temp1.x, temp1.y, &temp1.x, &temp1.y);
//			printf("INPUT COORDS: %f %f\n", temp1.x, temp1.y);
			inproj->forward(temp1.x, temp1.y, &temp1.x, &temp1.y);
//			printf("INPUT COORDS: %f %f\n", temp1.x, temp1.y);

			// temp now contains coords to input projection
			temp1.x -= in_ul.x;
			temp1.y += in_ul.y;
			temp1.x /= in_pixsize;
			temp1.y /= out_pixsize;
			// temp is now scaled to input raster coords, now resample!
			if ( in_rows - (int)temp1.y <= 0 ) {
			printf("Input size %d cols, %d rows\n",
			       in_cols, in_rows);
			printf("Sanity check: %d, %d\n",
			       ((unsigned int)temp1.x),
			       ((unsigned int)temp1.y));
			
			continue;
			}

			
/*
			resampler(&(inraster[0]), temp1.x, temp1.y, in_cols, 
				  &(outraster[0]), x, y, out_cols);
*/


			try {
				unsigned long ss = (long unsigned int)temp1.y;
				ss *= in_cols;
				ss += (long unsigned int)temp1.x;
				outraster.at(x + (y * out_cols)) =
					inraster.at(ss);
			} catch(std::exception) {
				s = out_rows;
				s *= out_cols;
				s *= output->bitsPerPixel()/8;
				
				printf("----------------------------------\n");
				printf("Error writing (%d %d) to (%d %d)\n",
				       (unsigned int)temp1.x, (unsigned int)temp1.y, x, y);
				printf("Input Raster: (%d cols, %d rows)\n"
				       "Output Raster: (%d cols, %d rows)\n",
				       in_cols, in_rows, out_cols, out_rows);
				printf("----------------------------------\n");

			}
		}
	}

	// Write output raster
	output->writeRaster(firstRow, numRows, &(outraster[0]));

	delete outproj;
	delete inproj;

	return;

}

void Reprojector::reproject()
{
	parallelReproject();

}

void FindMinBox(RasterChunk *input, RasterChunk *output)
{
	double ul_x, ul_y;
	double pixsize; // in METER!
	Coordinate temp;
	Transformer t;
	double maxx, minx, maxy, miny;
	double ul_lon, ul_lat, lr_lon, lr_lat;
	Projection *inproj = input->projection;
	Projection *outproj = output->projection;

	maxx = maxy = 0;
	minx = miny = 1e+37;

	ul_x = input->ul_x;
	ul_y = input->ul_y;
	pixsize = input->pixel_size;
  
	t.setInput(*(inproj->copy()));
	t.setOutput(*(outproj->copy()));

	// Find geographic corners of input
	inproj->inverse(ul_x, ul_y, &ul_lon, &ul_lat);
	inproj->inverse(ul_x+(input->num_cols*pixsize), 
					ul_y-(input->num_rows*pixsize), 
					&lr_lon, 
					&lr_lat);
	double delta_east = (lr_lon-ul_lon)/input->num_cols, 
		delta_north = (ul_lat-lr_lat)/input->num_rows;

	// Calculate minbox
	temp.x = ul_x;
	temp.y = ul_y;
	temp.units = METER;

	// Check top of map
	for (int x = 0; x < input->num_cols; ++x) {
		temp.x = (double)x*pixsize + ul_x;
		temp.y = ul_y;
		t.transform(&temp);
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
		temp.x = ul_lon + (x*delta_east);
		temp.y = ul_lat;
		outproj->forward(temp.x, temp.y, &(temp.x), &(temp.y));
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
	}

	// Check bottom of map
	for (int x = 0; x < input->num_cols; ++x) {
		temp.x = (double)x*pixsize + ul_x;
		temp.y = (double)-input->num_cols*pixsize + ul_y;
		t.transform(&temp);
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
		temp.x = ul_lon + (x * delta_east);
		temp.y = ul_lat - (input->num_rows*delta_north);
		outproj->forward(temp.x, temp.y, &(temp.x), &(temp.y));
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;

	}
 
	// Check Left side
	for (int y = 0; y < input->num_rows; ++y) {
		temp.x = ul_x;
		temp.y = (double)-y*(pixsize+1) + ul_y;
		t.transform(&temp);
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
		temp.x = ul_lon;
		temp.y = ul_lat - (y*delta_north);
		outproj->forward(temp.x, temp.y, &(temp.x), &(temp.y));
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
	}

	// Check right side
	for (int y = 0; y < input->num_rows; ++y) {
		temp.x = (double)input->num_cols*(1+pixsize) + ul_x;
		temp.y = (double)-y*pixsize + ul_y;
		t.transform(&temp);
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
		temp.x = ul_lon + (input->num_cols * delta_east);
		temp.y = ul_lat - (y * delta_north);
		outproj->forward(temp.x, temp.y, &(temp.x), &(temp.y));
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
	}

	// Set outputs
	output->ul_x = minx;
	output->ul_y = maxy;




}

void FindMinBox2(double in_ul_x, double in_ul_y,
		 double in_pix_size,
		 int rows, int cols, 
		 Projection *inproj,
		 Projection *outproj,
		 double out_pixsize,
		 double &_ul_x, double &_ul_y, double &_lr_x, double &_lr_y)
{
	double ul_x, ul_y;
	double pixsize; // in METER!
	Coordinate temp;
	Transformer t;
	double maxx, minx, maxy, miny;
	double ul_lon, ul_lat, lr_lon, lr_lat;

	maxx = maxy = 0;
	minx = miny = 1e+37;

	ul_x = in_ul_x;
	ul_y = in_ul_y;
	pixsize = out_pixsize;
  
	t.setInput(*(inproj->copy()));
	t.setOutput(*(outproj->copy()));

	// Find geographic corners of input
	inproj->inverse(ul_x, ul_y, &ul_lon, &ul_lat);
	inproj->inverse(ul_x+(cols*pixsize), 
					ul_y-(rows*pixsize), 
					&lr_lon, 
					&lr_lat);
	double delta_east = (lr_lon-ul_lon)/cols, 
		delta_north = (ul_lat-lr_lat)/rows;

	// Calculate minbox
	temp.x = ul_x;
	temp.y = ul_y;
	temp.units = METER;

	// Check top of map
	for (int x = 0; x < cols; ++x) {
		temp.x = (double)x*pixsize + ul_x;
		temp.y = ul_y;
		t.transform(&temp);
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
		temp.x = ul_lon + (x*delta_east);
		temp.y = ul_lat;
		outproj->forward(temp.x, temp.y, &(temp.x), &(temp.y));
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
	}

	// Check bottom of map
	for (int x = 0; x < cols; ++x) {
		temp.x = (double)x*pixsize + ul_x;
		temp.y = (double)-rows*pixsize + ul_y;
		t.transform(&temp);
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
		temp.x = ul_lon + (x * delta_east);
		temp.y = ul_lat - (rows*delta_north);
		outproj->forward(temp.x, temp.y, &(temp.x), &(temp.y));
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;

	}
 
	// Check Left side
	for (int y = 0; y < rows; ++y) {
		temp.x = ul_x;
		temp.y = (double)-y*(pixsize+1) + ul_y;
		t.transform(&temp);
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
		temp.x = ul_lon;
		temp.y = ul_lat - (y*delta_north);
		outproj->forward(temp.x, temp.y, &(temp.x), &(temp.y));
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
	}

	// Check right side
	for (int y = 0; y < rows; ++y) {
		temp.x = (double)cols*(1+pixsize) + ul_x;
		temp.y = (double)-y*pixsize + ul_y;
		t.transform(&temp);
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
		temp.x = ul_lon + (cols * delta_east);
		temp.y = ul_lat - (y * delta_north);
		outproj->forward(temp.x, temp.y, &(temp.x), &(temp.y));
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
	}

	// Set outputs
	_ul_x = minx;
	_ul_y = maxy;
	_lr_x = maxx;
	_lr_y = miny;


 	outproj->inverse(minx, maxy, &temp.x, &temp.y);
	outproj->inverse(maxx, miny);
//	printf("MINBOX ul %f %f lr %f %f or ul(%f %f) lr(%f %f)\n", minx, maxy, maxx, miny,
//	       temp.x, temp.y, outproj->lon(), outproj->lat());
  


}


// Minbox finds number of rows and columns in output and upper-left
// corner of output
void FindMinBox(ProjectedRaster *input, Projection *outproj, double out_pixsize,
		double &_ul_x, double &_ul_y, double &_lr_x, double &_lr_y)
{
	double ul_x, ul_y;
	double pixsize; // in METER!
	Coordinate temp;
	Transformer t;
	int cols, rows;
	double maxx, minx, maxy, miny;
	double ul_lon, ul_lat, lr_lon, lr_lat;

	maxx = maxy = 0;
	minx = miny = 1e+37;

	ul_x = input->ul_x;
	ul_y = input->ul_y;
	cols = input->getColCount();
	rows = input->getRowCount();
	pixsize = out_pixsize;
  
	t.setInput(*input->getProjection());
	t.setOutput(*outproj);

	// Find geographic corners of input
	input->getProjection()->inverse(ul_x, ul_y, &ul_lon, &ul_lat);
	input->getProjection()->inverse(ul_x+(cols*pixsize), 
					ul_y-(rows*pixsize), 
					&lr_lon, 
					&lr_lat);
	double delta_east = (lr_lon-ul_lon)/cols, 
		delta_north = (ul_lat-lr_lat)/rows;

	// Calculate minbox
	temp.x = ul_x;
	temp.y = ul_y;
	temp.units = METER;

	// Check top of map
	for (int x = 0; x < cols; ++x) {
		temp.x = (double)x*pixsize + ul_x;
		temp.y = ul_y;
		t.transform(&temp);
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
		temp.x = ul_lon + (x*delta_east);
		temp.y = ul_lat;
		outproj->forward(temp.x, temp.y, &(temp.x), &(temp.y));
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
	}

	// Check bottom of map
	for (int x = 0; x < cols; ++x) {
		temp.x = (double)x*pixsize + ul_x;
		temp.y = (double)-rows*pixsize + ul_y;
		t.transform(&temp);
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
		temp.x = ul_lon + (x * delta_east);
		temp.y = ul_lat - (rows*delta_north);
		outproj->forward(temp.x, temp.y, &(temp.x), &(temp.y));
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;

	}
 
	// Check Left side
	for (int y = 0; y < rows; ++y) {
		temp.x = ul_x;
		temp.y = (double)-y*(pixsize+1) + ul_y;
		t.transform(&temp);
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
		temp.x = ul_lon;
		temp.y = ul_lat - (y*delta_north);
		outproj->forward(temp.x, temp.y, &(temp.x), &(temp.y));
		if (temp.x < minx) minx = temp.x;
		if (temp.x > maxx) maxx = temp.x;
		if (temp.y < miny) miny = temp.y;
		if (temp.y > maxy) maxy = temp.y;
	}

	// Check right side
	for (int x = -1 * (cols*.1); x < cols*.1; ++x) {
		for (int y = 0; y < rows; ++y) {
			temp.x = ((double)cols+x)*(1+pixsize) + ul_x;
			temp.y = (double)-y*pixsize + ul_y;
			t.transform(&temp);
			if (temp.x < minx) minx = temp.x;
			if (temp.x > maxx) maxx = temp.x;
			if (temp.y < miny) miny = temp.y;
			if (temp.y > maxy) maxy = temp.y;
			temp.x = ul_lon + (cols * delta_east);
			temp.y = ul_lat - (y * delta_north);
			outproj->forward(temp.x, temp.y, &(temp.x), &(temp.y));
			if (temp.x < minx) minx = temp.x;
			if (temp.x > maxx) maxx = temp.x;
			if (temp.y < miny) miny = temp.y;
			if (temp.y > maxy) maxy = temp.y;
		}
	}

	// Set outputs
	_ul_x = minx;
	_ul_y = maxy;
	_lr_x = maxx;
	_lr_y = miny;
  

	return;
}

