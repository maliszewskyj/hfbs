package require BLT
source /usr/lib/blt2.4/bltGraph.tcl
load libaryimg.so

blt::vector y(4096)
blt::vector xs(4096)

for {set i 0} {$i < 4096} {incr i} { set xs($i) $i }
blt::graph .g
.g element create e0 -xdata xs -ydata y -symbol none
pack .g
blt::ZoomStack .g
wm deiconify .