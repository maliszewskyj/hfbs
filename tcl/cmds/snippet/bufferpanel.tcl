#!/usr/bin/wish

# Fill array buffer
#
proc BufferCreate { } {
    global buffer
    set buffer(nbuf) 5
    set buffer(colors) {black red blue green grey}
    for {set i 0} {$i < $buffer(nbuf)} {incr i} {
	set buffer($i,total) total$i
	set buffer($i,data)  data$i
	set buffer($i,norm)      norm$i
	set buffer($i,tottime)   0.0
	set buffer($i,filename)  none
	set buffer($i,comments)  empty
	set buffer($i,starttime) "01-Jan-1970 00:00:00"
	set buffer($i,livetime)  0
	set buffer($i,mode)      teach
	set buffer($i,show)      0
    }
}

proc BufferPanelBuild { f } {
    global buffer

    if [catch {frame $f} result] {
	return -code error $result
    }

    label $f.numlab  -text "#"
    label $f.plotlab -text "Plot"
    label $f.filelab -text "Filename"  -width 20
    label $f.commlab -text "Comments"  -width 40
    label $f.timelab -text "Time(sec)" -width 10
    label $f.loadlab -text "Load"
    label $f.delelab -text "Delete"

    grid $f.numlab  -row 0 -column 0
    grid $f.plotlab -row 0 -column 1
    grid $f.filelab -row 0 -column 2 -sticky w
    grid $f.commlab -row 0 -column 3 -sticky w
    grid $f.timelab -row 0 -column 4 -sticky w
    grid $f.loadlab -row 0 -column 5
    grid $f.delelab -row 0 -column 6

    for {set i 0} {$i < $buffer(nbuf)} {incr i} {
	label $f.num$i -text $i -foreground [lindex $buffer(colors) $i]
	checkbutton $f.plot$i -variable buffer($i,show) -text ""
	label $f.file$i -textvariable buffer($i,filename) -anchor w
	label $f.comm$i -textvariable buffer($i,comments) -anchor w
	label $f.time$i -textvariable buffer($i,livetime) -anchor e
	button $f.load$i -text "X" -padx 0 -pady 0 -command "OpenData $i"
	button $f.dele$i -text "X" -padx 0 -pady 0 -command "BufferDelete $i"
	set row [expr $i + 1]
	grid $f.num$i  -row $row -column 0 
	grid $f.plot$i -row $row -column 1
	grid $f.file$i -row $row -column 2 -sticky w
	grid $f.comm$i -row $row -column 3 -sticky w
	grid $f.time$i -row $row -column 4 -sticky e
	grid $f.load$i -row $row -column 5
	grid $f.dele$i -row $row -column 6
    }
    return $f
}

proc BufferDelete { i } {
    puts "Deleting contents of buffer $i"
}





