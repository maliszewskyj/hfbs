#!/usr/bin/perl
#
# Calculate flight time offset correction
#       All distances are reported in meters
#       All times are reported in seconds

$usage = 
 "Usage: gen_ftoff [-m] [-M <d_ms>] [-S <d_sm>] [-s <d_sad>] [-v <v_bragg>]\n".
 "   -o                            calculate for OR ADCs\n".
 "   -m                            calculate table for monitor\n".
 "   -M <d_ms>                     monochromator-sample distance\n".
 "   -S <d_sm>                     sample-monitor distance\n".
 "   -s <d_sad>                    sample-analyser-detector distance\n".
 "   -v <v_bragg>                  velocity of incident neutrons\n".
 "   specify distances in meters and velocities in meters/sec\n"

require "getopts.pl";
&Getopts('mM:s:S:v:b:');

$debug = 0;
$detmon = 1; # detectors if 1, monitor if 0
$v_bragg = 630.825;
$d_ms  = 2.25;
$d_sad = 4.10;
$d_sm  = 2.00;
$t_bin = 20e-6;
$maxdrange = 0.082;
$d_off=0;
$v_off=0;

# Process command line options
if ($opt_m) { $detmon = 0; } # Do calculation for monitor
if ($opt_M) { $d_ms   = 1.0 * $opt_M; } 
if ($opt_S) { $d_sm   = 1.0 * $opt_S; }
if ($opt_s) { $d_sad  = 1.0 * $opt_s; }
if ($opt_v) { $v_bragg= 1.0 * $opt_v; }

if ($detmon) { 
    $d_fix = $d_ms; 
} else {
    $d_fix = $d_ms + $d_sm;
}
$t_elastic = $d_fix / $v_bragg;
$d_scale = (0.082 * (10.0/8.0)) / 2048;
$v_scale = 20.0 / 4096;

if ($debug) { 
    if ($detmon) { 
	$t_flight = ($d_ms + $d_sad) / $v_bragg;
    } else {
	$t_flight = ($d_ms + $d_sm) / $v_bragg;
    }
    $t_fix = int( $t_flight / $t_bin );
    print "t_flight = $t_flight\n";
    print "t_fix    = $t_fix\n";
}

for($i=0;$i<256;$i++) {
    $v_idx = (($i<<4) - 2048);
    $delta_v = $v_idx * $v_scale;
    for ($j=0;$j<256;$j++) {
	$d_idx = (($j<<4) - 2048);
	$delta_d = $d_idx * $d_scale;
	$t_flight = ($d_fix - $delta_d) / ($v_bragg + $delta_v);
	$offset = int(($t_flight - $t_elastic) / $t_bin);
	if ($debug) { printf "%3d %3d ",$i,$j;}
	printf " %d \n",$offset;
    }
}

