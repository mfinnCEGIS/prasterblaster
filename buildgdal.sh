#!/bin/sh

OLDDIR=$(pwd)
PRBROOT=$(dirname $0)

cd $PRBROOT
PRBROOT=$(pwd)
cd $OLDDIR

mkdir $PRBROOT/src/gdal
cd $PRBROOT/src/gdal/


if command -v gmake 2>/dev/null; then
    MAKE=gmake
else
    MAKE=make
fi

if command -v wget 2>/dev/null; then
    WGET="wget -c"
else
    WGET="ftp -C"
fi

# Build proj4
$WGET http://download.osgeo.org/proj/proj-4.8.0.tar.gz
rm -rf proj-4.8.0
tar xfz proj-4.8.0.tar.gz
cd proj-4.8.0
./configure --prefix=$PRBROOT/src/gdal/ --without-jni --without-mutex
$MAKE -j2
$MAKE install

# Build GDAL
cd $PRBROOT/src/gdal/
$WGET http://download.osgeo.org/gdal/gdal-1.9.2.tar.gz
rm -rf gdal-1.9.2
tar xfz gdal-1.9.2.tar.gz
cd gdal-1.9.2/
./configure --prefix=$PRBROOT/src/gdal/ --with-libtiff=internal --with-geotiff=internal \
--with-jpeg=internal \
--with-liblzma=no \
--with-pg=no \
--with-grass=no \
--with-libgrass=no \
--with-cfitsio=no \
--with-pcraster=no \
--with-png=no \
--with-gta=no \
--with-pci-disk=no \
--with-gif=no \
--with-ogdi=no \
--with-fme=no \
--with-hdf4=no \
--with-hdf5=no \
--with-netcdf=no \
--with-jasper=no \
--with-openjpeg=no \
--with-fgdb=no \
--with-ecw=no \
--with-kakadu=no \
--with-mrsid=no \
--with-jp2mrsid=no \
--with-mrsid_lidar=no \
--with-msg=no \
--with-ingres=no \
--with-xerces=no \
--with-expat=no \
--with-libkml=no \
--with-odbc=no \
--with-dods-root=no \
--with-curl=no \
--with-xml2=no \
--with-spatialite=no \
--with-sqlite3=no \
--with-pcre=no \
--with-dwgdirect=no \
--with-idb=no \
--with-sde=no \
--with-epsilon=no \
--with-webp=no \
--with-opencl=no \
--with-freexl=no \
--with-poppler=no \
--with-podofo=no \
--with-perl=no \
--with-php=no \
--with-ruby=no \
--with-python=no \
--with-java=no \
--with-mdb=no \
--with-radaman=no \
--with-armadillo=no \
--with-libz=internal \
--with-grib=no



$MAKE -j2
$MAKE install

# Create library link if necessary
ln -s $PRBROOT/src/gdal/lib/libgdal.so.* $PRBROOT/src/gdal/lib/libgdal.so

cd $OLDDIR
