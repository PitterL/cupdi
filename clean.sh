#!/bin/sh

make clean
find ./ -name Makefile -delete
find ./ -name Makefile.in -delete
find ./ -name "*.o" -delete
find ./ -name "*.a" -delete

