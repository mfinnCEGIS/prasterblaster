
#include <cstdio>
#include <vector>

#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>

#include "gctp_cpp/projection.h"
#include "gctp_cpp/transformer.h"
#include "gctp_cpp/coordinate.h"

#include "reprojector.hh"
#include "resampler.hh"

namespace mpi = boost::mpi;

Reprojector::Reprojector(ProjectedRaster *_input, ProjectedRaster *_output) :
  input(_input), output(_output)
{
	maxx = maxy = 0;
	minx = miny = 1e+37;

	if (input->getBitCount() == 8) {
		resampler = &nearest_neighbor<unsigned char>;
	}

	return;
};

Reprojector::~Reprojector()
{
  
  
	return;
}

void Reprojector::parallelReproject(int rank, int numProcs)
{
	// We assume input is filled with raster goodness
	Transformer t;
	Coordinate temp, temp2;
	double in_ulx, in_uly, out_ulx, out_uly;
	double in_pixsize, out_pixsize;
	long in_rows, in_cols, out_rows, out_cols;
	mpi::communicator world;
	vector<char> ot;

	t.setInput(*output->getProjection());
	t.setOutput(*input->getProjection());
  
	in_rows = input->getRowCount();
	in_cols = input->getColCount();
	out_rows = output->getRowCount()/numProcs;
	out_cols = output->getColCount();
	in_pixsize = input->getPixelSize();
	out_pixsize = output->getPixelSize();
	in_ulx = input->ul_x;
	in_uly = input->ul_y;
	out_ulx = output->ul_x;
	out_uly = output->ul_y - (rank * ((out_rows * out_pixsize)/numProcs));

	temp.units = METER;
	for (int y = 0; y < out_rows; ++y) {
		if (y % 100 == 0)
			printf("Node %d On row %d of %d\n", rank, 
			       y + (rank * (output->getRowCount()/numProcs)), output->getRowCount());
		for (int x = 0; x < out_cols; ++x) {
			temp.x = ((double)x * out_pixsize) + out_ulx;
			temp.y = ((double)y * out_pixsize) - out_uly;
			t.transform(&temp);
			output->getProjection()->inverse(temp.x, temp.y);
			output->getProjection()->forward(output->getProjection()->lon(),
							 output->getProjection()->lat(), 
							 &(temp2.x), &(temp2.y));
			temp.x = ((double)x * out_pixsize) + out_ulx;	
			temp.y = ((double)y * out_pixsize) - out_uly;

			t.transform(&temp);

			// temp now contains coords to input projection
			temp.x -= in_ulx;
			temp.y += in_uly;
			temp.x /= in_pixsize;
			temp.y /= out_pixsize;
			// temp is now scaled to input raster coords, now resample!
			resampler(input->data, temp.x, temp.y, in_cols, 
				  output->data, x, y, out_cols);

		}
	}

	mpi::gather(world, (char*)output->data, out_rows * out_cols, (char*)output->data, 0);
	
	return;
}

void Reprojector::reproject()
{
	parallelReproject(0, 1);

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
	cols = input->cols;
	rows = input->rows;
	pixsize = input->getPixelSize();
  
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
  

	return;
}

ProjectedRaster* GetOutputRaster(ProjectedRaster* input,
				 Projected *output_proj,
				 double output_pixel_size)
{
	double ul_x, ul_y, lr_x, lr_y;
	long num_rows, num_cols;
	
	ul_x = ul_y = lr_x = lr_y = 0d0;
	FindMinBox(input, output_proj, output_pix_size, ul_x, ul_y, lr_x, 
		   lr_y);
	num_rows = (ul_y - lr_y) / output_pixel_size;
	num_cols = (lr_x - ul_x) / output_pixel_size;
	


	return new ProjectedRaster(num_rows, num_cols, input->getBitCount(),
				   output_proj->copy(), ul_x, ul_y);
}

vector<ProjectedRaster*> PartitionRaster(ProjectedRaster *input, 
					 Projection *output_proj, 
					 double output_pix_size, 
					 int partition_count)
{
	double ul_x, ul_y, lr_x, lr_y;
	double part_height, part_rows;
	vector<ProjectedRaster*> parts(partition_count);
	

	ul_x = ul_y = lr_x = lr_y = part_height = 0d0;

	FindMinBox(input, output_proj, output_pix_size, ul_x, ul_y, lr_x, 
		   lr_y);
	part_height = (lr_y - ul_y) / partition_count;
	part_rows = part_height / output_pix_size;
	
	for (int i = 0; i < partition_count; ++i) {
		parts[i] = new ProjectedRaster(part_rows,
					       input->getColCount(),
					       input->getBitCount(),
					       output_proj,
					       ul_x,
					       ul_y + i*part_height);
	}

	return parts;
}
