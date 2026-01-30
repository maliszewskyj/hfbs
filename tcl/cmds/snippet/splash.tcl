#!/usr/bin/wish

wm withdraw .
toplevel .placard -borderwidth 4 -relief raised
wm overrideredirect .placard 1

after idle {
    update idletasks
    set xmax [winfo screenwidth .placard]
    set ymax [winfo screenheight .placard]
    set x0 [expr ($xmax - [winfo reqwidth .placard])/2]
    set y0 [expr ($ymax - [winfo reqheight .placard])/2]
    wm geometry .placard "+$x0+$y0"
}

label .placard.info \
	-text "National Institute of Standards and Technology"
pack .placard.info -side bottom -fill x
set imh [image create photo -file ncnrtrans.gif]
label .placard.icon -image $imh
pack .placard.icon -side top 
update

#
# setup application

#
#
# finally...
#
# update
# destroy .placard
# wm deiconify .