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
 * Tests for RasterCoordTransformer class
 *
 */

#include <gtest/gtest.h>

#include <vector>

#include "src/projectedraster.h"
#include "src/reprojection_tools.h"
#include "src/rastercoordtransformer.h"
#include "src/gctp_cpp/transformer.h"

using librasterblaster::ProjectedRaster;
using librasterblaster::RasterCoordTransformer;
using librasterblaster::Area;
using std::vector;

TEST(RasterCoordTransformer, EdgeTransformations) {
  return;
}
