set runinfo(rundir)         /home/hfbs/data
set runinfo(runfile)        19981101_01.hfbs
set runinfo(mode)           teach
set runinfo(user)           ""
set runinfo(starttime)      [clock format [clock seconds] -format "%d-%b-%Y %H:%M:%S"]
set runinfo(ncycles)        999
set runinfo(cyctime)        600
set runinfo(curcycle)       0
set runinfo(cycstart)       -1
set runinfo(tottime)        "000000"
set runinfo(comments)       "empty"
set runinfo(doppler)        " 10.0"
set runinfo(sampletemp)     "300.0"
set runinfo(status)         "Progress 0/999"

proc GetRunInfo { } {
    global runinfo

    if [catch {Launch array get run} results] {
	return
    }
    array set remoterun $results
    foreach f [array names remoterun] {
	set runinfo($f) $remoterun($f)
    }
    set runinfo(starttime) [clock format $remoterun(starttime) \
	    -format "%d-%b-%Y %H:%M:%S"]
    set runinfo(status) "Progress $runinfo(curcycle)/$runinfo(ncycles)"
}

#
# Build status panel for displaying run information
#
proc StatusPanelBuild { parent } {
    global runinfo 
    label $parent.filename -textvariable runinfo(runfile) -width 20 \
	    -foreground blue -anchor w
    label $parent.comments -textvariable runinfo(comments) \
	    -foreground blue -anchor w
    label $parent.mode     -textvariable runinfo(mode) -foreground blue
    label $parent.startlab -text "Start:"
    label $parent.start    -textvariable runinfo(starttime) -foreground blue
    label $parent.curstat  -textvariable runinfo(status) -foreground red
    label $parent.timelab  -text "Time (sec):"
    label $parent.time     -textvariable runinfo(tottime) -foreground blue
    label $parent.dopplerl -text "Doppler (Hz):"
    label $parent.doppler  -textvariable runinfo(doppler) -foreground blue \
	    -anchor e
    label $parent.sampletl -text "Tsample  (K):"
    label $parent.samplet  -textvariable runinfo(sampletemp) -foreground blue \
	    -anchor e
    grid $parent.filename -row 0 -column 0 -sticky w
    grid $parent.startlab -row 0 -column 1 -sticky w
    grid $parent.start    -row 0 -column 2 -sticky w
    grid $parent.timelab  -row 0 -column 3 -sticky w
    grid $parent.time     -row 0 -column 4 -sticky w
    grid $parent.comments -row 1 -columnspan 5 -sticky w
    grid $parent.mode     -row 2 -column 0 -sticky w
    grid $parent.curstat  -row 3 -column 0 -sticky w
    grid $parent.dopplerl -row 0 -column 5 -sticky w 
    grid $parent.doppler  -row 0 -column 6 -sticky e
    grid $parent.sampletl -row 1 -column 5 -sticky w
    grid $parent.samplet  -row 1 -column 6 -sticky e
}

proc BuildGUI { } {
    pack [frame .f]
    StatusPanelBuild .f
}