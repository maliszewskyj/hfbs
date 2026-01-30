#!/usr/bin/wish

set scan(npts)     1
set scan(pttime)   10
set scan(scmode)   time
set scan(tstart)   300.0
set scan(deltat)   0.0
set scan(holdsec)  60
set scan(comment)  ""
set scan(toplevel) ""

proc ScanDialogBuild { parent } {
    global scan

    toplevel $parent
    set scan(toplevel) $parent

    set nl [label $parent.nptlab -text "Number of  pts:"]
    set ne [entry $parent.nptent -width 12 -textvariable scan(npts)]
    set pl [label $parent.pttlab -text "Time/pt (sec):"]
    set pe [entry $parent.pttent -width 12 -textvariable scan(pttime)]
    set ml [label $parent.modlab -text "Scan mode:"]
    tk_optionMenu $parent.mode scan(scmode) time temp
    set me $parent.mode
    $me configure -width 8
    set tl [label $parent.tstlab -text "T start (K):"]
    set te [entry $parent.tstent -width 12 -textvariable scan(tstart)]
    set il [label $parent.inclab -text "T increment:"]
    set ie [entry $parent.incent -width 12 -textvariable scan(deltat)]
    set hl [label $parent.hldlab -text "Hold/pt (sec):"]
    set he [entry $parent.hldent -width 12 -textvariable scan(holdsec)]
    set bs [button $parent.start -text "Start" -command ScanStart]
    set bd [button $parent.dismiss -text "Dismiss" -command ScanDialogHide]

    grid $nl $ne -sticky w
    grid $pl $pe -sticky w
    grid $ml $me -sticky w
    grid $tl $te -sticky w
    grid $il $ie -sticky w
    grid $hl $he -sticky w
    grid $bs $bd -sticky ew
    return $scan(toplevel)
}

proc ScanDialogShow { } {
    global scan
    wm deiconify $scan(toplevel)
}

proc ScanDialogHide { } {
    global scan
    wm withdraw $scan(toplevel)
}

proc ScanStart { } {
    global scan
    switch $scan(scmode) {
	time {
	    catch {Launch scan start $scan(npts) $scan(pttime) time \
		    $scan(comments)} result
	}
	temp {
	    catch {Launch scan start $scan(npts) $scan(pttime) temp \
		    $scan(tstart) $scan(deltat) $scan(holdsec) \
		    $scan(comments)} result
	}
	default {
	    return
	}
    }
    PutText $result
}

proc ScanStop { } {
    catch {Launch scan abort} result
    PutText $result
}

###############################################################################
wm withdraw .

ScanDialogBuild .scan
ScanDialogShow