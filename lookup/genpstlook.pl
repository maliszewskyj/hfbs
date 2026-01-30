#!/usr/bin/perl

$limit = 4096;
$pi = atan2(1,1) * 4;

$period = 2 * $pi * 3 / $limit;
$detoff = 0;
$monoff = 100;

for ($i=0;$i<$limit;$i++) {
    $detarg = $period * ($i - $detoff);
    $monarg = $period * ($i - $monoff);
    $detcos = cos($detarg);
    $moncos = cos($monarg);
    $lookup = 1;
#    $lookup = 0;
#    if ($detcos >= 0) {
#	$lookup += 2;
#    } 
    if ($moncos >= 0) {
	$lookup += 4;
    }
    printf " %x \n",$lookup;
}
