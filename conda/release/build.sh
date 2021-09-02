#!/bin/bash
cmake -Bbuild -H. -G "Ninja" -DLIMA_BUILD_SUBMODULES=0 -DLIMA_ENABLE_PYTHON=1 -DLIMA_ENABLE_TESTS=1 -DLIMA_ENABLE_DEBUG=1 -DLIMA_ENABLE_CONFIG=1 -DLIMA_ENABLE_TIFF=1 -DLIMA_ENABLE_HDF5=1 -DLIMA_ENABLE_HDF5_BS=1 -DLIMA_ENABLE_EDFGZ=1 -DLIMA_ENABLE_EDFLZ4=1 -DLIMA_ENABLE_CBF=1 -DLIMA_ENABLE_NUMA=1 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=$PREFIX -DPYTHON_SITE_PACKAGES_DIR=$SP_DIR -DCMAKE_FIND_ROOT_PATH=$PREFIX -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY
cmake --build build --target install
