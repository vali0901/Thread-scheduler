#!/bin/bash

cd ./so/assignments/2-scheduler/util/
make clean
make 
cp libscheduler.so ../checker-lin/ &&
cd ../checker-lin &&
make -f Makefile.checker clean
make -f Makefile.checker

if [ $# -ge 1 ]
then
	LD_LIBRARY_PATH=. ./_test/run_test $1
fi
