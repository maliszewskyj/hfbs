#!/usr/bin/tclsh
#package require BLT 
load libNArray.so
#load libaryimg.so
load ./libacquire.so ;# Load data acquisition shared library
load libmtxutil.so

# Create narrays used for data storage and manipulation 
if [catch {narray create increment 20 4096 } result] {
    puts stderr "Could not create narray \"increment\". Exiting immediately!"
    exit
}

if [catch {narray create total 20 4096} result] {
    puts stderr "Could not create narray \"total\". Exiting immediately!"
    exit
}

if [catch {narray create work2 1 1} result] {
    puts stderr "Could not create narray \"work2\". Exiting immediately!"
    exit
}

if [catch {narray create work1 1} result] {
    puts stderr "Could not create narray \"work1\". Exiting immediately!"
    exit
}

if [catch {narray create counts 1 1} result] {
    puts stderr "Could not create narray \"counts\". Exiting immediately!"
    exit
}

if [catch {narray create pv 1 1} result] {
    puts stderr "Could not create narray \"pv\". Exiting immediately!"
    exit
}

# calculate energies associated with each velocity channel
if [catch {narray create energy 1  4096} result] {
    puts stderr "Could not create narray \"energy\". Exiting immediately!"
    exit
}
energy vset k1 5.22707e-3
energy vset k2 1261.64
energy vset v 0.0
energy vset erange 200.0
#energy map {
#    v = (@1 - 2048.0)/204.8;
#    [] = $k1 * ($v * $v + $k2 * $v);
#}
energy map { 
     e = (@1 - 2048.0) / 2048.0;
     [] = $e * $erange;
}

if [catch {narray create ordinal 1 4096} result] {
    puts stderr "Could not create narray \"ordinal\". Exiting immediately!"
    exit
}
ordinal map { [] = @1; }
