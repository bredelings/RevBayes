#!/bin/sh
if [ "$1" = "clean" ]
then
	rm -rf build
else
if [ ! -d build ]; then
	mkdir build
fi
    if [ -n "${CC}" ] ; then echo "Using '${CC}' as C compiler." ; fi
    if [ -n "${CXX}" ] ; then echo "Using '${CXX}' as C++ compiler." ; fi
    echo
    #################
    # generate git version number
    ./generate_version_number.sh
    cp ../../src/revlanguage/utils/GitVersion.cpp GitVersion_backup.cpp
    mv GitVersion.cpp ../../src/revlanguage/utils/

	./regenerate.sh $@
	cd build
	cmake .
	make -j 4
	cd ..
	
    cp GitVersion_backup.cpp ../../src/revlanguage/utils/GitVersion.cpp
    rm GitVersion_backup.cpp
fi
