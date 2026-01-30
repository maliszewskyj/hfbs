#!/usr/bin/tclsh
package require narray
narray create total 23 2048
narray create counts 1 1
narray create energy 1 1
narray create pv 1 1 
narray create ndata 23 1
ndata map {[] = @0;}
# Set up to emulate a scan
# Fill array with -1
total map {[] = -1;}
for {set i 0} {$i < 10} {incr i} { total import [list 0 $i] ndata }
load /home/nickm/source/hfbs/daq/libmtxutil.so
mtx split scan total counts pv energy