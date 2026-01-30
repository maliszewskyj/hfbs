#!/usr/bin/perl

for ($i=0;$i<4096;$i++) {
#    if ($i & 0x800) {
#	$j = $i & 0x7ff;
#	$j = 2048 - $j;
#	$val = $j / 2;
#    } else {
#	$j = $i + 2048;
#	$val = $j / 2;
#    }
#    # printf " %d %d\n",$i,$val;
    $val = $i >> 1;
    printf " %03x \n",$val;
}
