#!/usr/bin/perl
#
# Calculate flight time offset correction
#       All distances are reported in meters
#       All times are reported in seconds


$debug = 0;
$detmon = 0; # detectors if 1, monitor if 0
$v_bragg = 630.825;
$d_ms  = 2.25;
$d_sad = 4.10;
$d_sm  = 2.00;
$t_bin = 20e-6;
$maxdrange = 0.082;
$d_off=0;
$v_off=0;

if ($detmon) { 
    $d_fix = $d_ms; 
} else {
    $d_fix = $d_ms + $d_sm;
}
$t_elastic = $d_fix / $v_bragg;
$d_scale = (0.082 * (10.0/8.0)) / 2048;
$v_scale = 20.0 / 4096;

# Read 12-bit ADC values 

while(<>) {
    ($vraw,$draw) = /\s*(\S+)\s+(\S+)/;
    $vraw = $vraw * 1;;
    $v_idx = (($vraw << 1) - 2048);
    $d_idx = ($draw * 1 - 2048);

    $delta_v = $v_idx * $v_scale;

    $delta_d = $d_idx * $d_scale;
    $t_flight = ($d_fix - $delta_d) / ($v_bragg + $delta_v);
    $offset = int(($t_flight - $t_elastic) / $t_bin);
    printf "%4d ",$vraw;
    #printf "%4d ",$draw;
    printf "%3d\n",$offset;
}


