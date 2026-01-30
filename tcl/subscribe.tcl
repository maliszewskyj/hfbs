#!/usr/bin/tclsh

# Subscribe - register client to receive status broadcasts from core program
#           - should perhaps subscribe only to certain services
#
proc Subscribe { } {
    global comm 

    # Cancel any previous subscriptions
    foreach c $comm(clients) {
	if { $c == $comm(current) } {
	    set comm(clients) [ldelete $comm(clients) $comm(current)]
	}
    }

    lappend comm(clients) $comm(current)
    return "OK:\nAutomatic updates enabled."
}

proc UnSubscribe { } {
    global comm 

    # Cancel any previous subscriptions
    set comm(clients) [ldelete $comm(clients) $comm(current) ]
    return "OK:\nAutomatic updates disabled."
}
