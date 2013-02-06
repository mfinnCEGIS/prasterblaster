/*!
 * Copyright 0000 <Nobody>
 * @file
 * @author David Matthew Mattli <dmattli@usgs.gov>
 *
 * @section LICENSE
 *
 * This software is in the public domain, furnished "as is", without
 * technical support, and with no warranty, express or implied, as to
 * its usefulness for any purpose.
 *
 * @section DESCRIPTION
 *
 * Header for the Simple Parallel Tiff Writer
 *
 */


#ifndef SRC_DEMOS_SPTW_H_
#define SRC_DEMOS_SPTW_H_

#include <gdal_priv.h>
#include <mpi.h>
#include <stdint.h>

#include <string>

using std::string;

/*! Simple parallel tiff writer namespace
 *
 * The functions declared here allow certain tiff files to be written to in
 * parallel by multiple processors by MPI-IO.
 *
 * Sequential consistency is guaranteed if the tiff file being written to is
 * stored row-wise, uses pixel interleaving and has its strips store
 * sequentially in the file, and no process writes to the same pixel
 * values. Files created with the create_raster function or with the
 * librasterblaster::ProjectedRaster::CreateRaster function should have the
 * required attributes.
 *
 * Two functions are provided to write to tiff files: write_rows and
 * ::write_subrow . write_rows allows multiple rows to be written at
 * once. ::write_subrows writes a portion of a row.
 *
 */
namespace sptw {
/*!
 * This enum lists the possible errors used by sptw.
 */
enum SPTW_ERROR {
  SP_None, /*!< No Error occurred */
  SP_CreateError, /*!< Error creating the file */
  SP_WriteError, /*!< Error Writing to the file */
  SP_BadArg, /*!< A bad argument was provided */
};

/**
 * @struct PTIFF sptw.h
 * @brief PTIFF struct represents an open parallel tiff file
 *
 *
 *
 */
struct PTIFF {
  /*! MPI-IO file handle. This should be changed with open_raster or
   *  close_raster */
  MPI_File fh;
  /*! Size of the opened raster in the x dimension */
  int x_size;
  /*! Size of the opened raster in the y dimension */
  int y_size;
  /*! Number of bands in the raster */
  int band_count;
  /*! Datatype of the band values */
  GDALDataType band_type;
  /*! Size in bytes of the band_type */
  int band_type_size;
  /*! Byte offset to the first strip of the tiff file */
  uint32_t first_strip_offset;
};

SPTW_ERROR create_raster(string filename,
                         int x_size,
                         int y_size,
                         int band_count,
                         double ul_x,
                         double ul_y,
                         double pixel_size,
                         GDALDataType band_type,
                         string projection_srs);

PTIFF* open_raster(string filename);
SPTW_ERROR close_raster(PTIFF *ptiff);
/**
 * @brief Writes in parallel one or more rows to raster in a sequentially
 * consistent way.
 *
 * @param ptiff Pointer to PTIFF struct representing the open file.
 */
SPTW_ERROR write_rows(PTIFF *ptiff,
                      void *buffer,
                      int64_t first_row,
                      int64_t last_row);
SPTW_ERROR write_subrow(PTIFF *ptiff,
                        void *buffer,
                        int64_t row,
                        int64_t first_column,
                        int64_t last_column);
}

#endif  // SRC_DEMOS_SPTW_H_