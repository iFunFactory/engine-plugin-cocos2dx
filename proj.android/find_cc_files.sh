#!/bin/sh
find -L -d ../Classes -name \*.cc -print | ./mkHelper.sh > out_cc_files.mk
