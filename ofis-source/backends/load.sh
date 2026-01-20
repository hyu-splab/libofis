#!/bin/bash
mkdir build;
cd build;
cmake ..;
make -j;

mv api/libdpu.so.0.0 api/libofis.so
cp api/libofis.so $UPMEM_HOME/lib/
cp api/libdpujni.so.0.0 $UPMEM_HOME/lib/

echo $UPMEM_HOME