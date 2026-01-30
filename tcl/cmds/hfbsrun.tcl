#!/usr/bin/wish
# $Id: hfbsrun.tcl,v 1.1.1.1 1999/09/20 14:32:09 nickm Exp $
#

###############################################################################
# Global variables
#
set tcl_precision 4
set hfbsbuf(nbuf) 5
set hfbsbuf(flds) { comment user ncyc cyctime t0 dt mode }
for {set i 0} {$i < $hfbsbuf(nbuf)} {incr i} {
    set hfbsbuf($i,comment)  empty
    set hfbsbuf($i,user)     ""
    set hfbsbuf($i,ncyc)      1
    set hfbsbuf($i,cyctime) 600
    set hfbsbuf($i,t0)      300.0
    set hfbsbuf($i,dt)        0.0
    set hfbsbuf($i,mode)    standard
}

foreach f $hfbsbuf(flds) { set hfbsbufdisp($f) $hfbsbuf(0,$f) }
set hfbsbufdisp(label)   "IBuffer No:  0"
set hfbsbufdisp(current)               0
set hfbsbufdisp(lbox)    .l.list

###############################################################################
# App specific functions
#
proc HFBSBufInit {} {
    global hfbsbuf hfbsbufdisp
}

proc HFBSBufDisp { recno } {
    global hfbsbufdisp hfbsbuf
    foreach fld $hfbsbuf(flds) {
	set icpdisp($fld) $hfbsbuf($recno,$fld)
    }
    set num [format "%2d" $recno]
    set hfbsdisp(label) "IBuffer No: $num"
    set hfbsdisp(current) $recno
}

proc HFBSBufEntryCreate { parent } {
    global hfbsbufdisp
    
    label $parent.bufnolabel -textvariable hfbsbufdisp(label)
    grid $parent.bufnolabel -row 0 -column 0
    label $parent.commentlabel -text "Comment:"
    grid $parent.commentlabel -row 0 -column 1 -sticky w
    entry $parent.comment -width 40 -textvariable hfbsbufdisp(comment)
    grid $parent.comment -row 0 -column 2 -columnspan 4 -sticky ew
    label $parent.userlabel -text "User:"
    grid $parent.userlabel -row 1 -column 1 -sticky w
    entry $parent.user -width 40 -textvariable hfbsbufdisp(user)
    grid $parent.user -row 1 -column 2 -columnspan 3 -sticky ew
    label $parent.ncyclabel -text "N cycles:"
    grid $parent.ncyclabel -row 2 -column 1 -sticky w
    entry $parent.ncyc -width 10 -textvariable hfbsbufdisp(ncyc)
    grid $parent.ncyc -row 2 -column 2 -sticky w
    label $parent.cyctimelabel -text "Cycle time:"
    grid $parent.cyctimelabel -row 2 -column 3 -sticky w
    entry $parent.cyctime -width 10 -textvariable hfbsbufdisp(cyctime)
    grid $parent.cyctime -row 2 -column 4 -sticky e
    label $parent.t0label -text "T0:"
    grid $parent.t0label -row 3 -column 1 -sticky w
    entry $parent.t0 -width 10 -textvariable hfbsbufdisp(t0)
    grid $parent.t0 -row 3 -column 2 -sticky w
    label $parent.dtlabel -text "Delta-T:"
    grid $parent.dtlabel -row 3 -column 3 -sticky w
    entry $parent.dt -width 10 -textvariable hfbsbufdisp(dt)
    grid $parent.dt -row 3 -column 4 -sticky e
    radiobutton $parent.modes -text Standard -variable hfbsbufdisp(mode) \
		-value standard
    grid $parent.modes -row 4 -column 1
    radiobutton $parent.modeT -text Tempscan -variable hfbsbufdisp(mode) \
		-value tscan -state disabled
    grid $parent.modeT -row 4 -column 2
    radiobutton $parent.modet -text Teach -variable hfbsbufdisp(mode) \
		-value teach
    grid $parent.modet -row 4 -column 3
    radiobutton $parent.modeh -text Hybrid -variable hfbsbufdisp(mode) \
		-value hybrid -state disabled
    grid $parent.modeh -row 4 -column 4
    button $parent.accept  -text "Accept"  -command HFBSBufUpdate
    grid $parent.accept -row 5 -column 0
    button $parent.dismiss -text "Dismiss" -command HFBSBufEditHide
    grid $parent.dismiss -row 5 -column 4 
    return $parent
}

proc HFBSBufList { } { 
    global hfbsbufdisp hfbsbuf
    # Clear out our list if it contains anything.
    set lbox $hfbsbufdisp(lbox)
    $lbox delete 0 end
    $lbox configure -font fixed
    for {set i 0} {$i < $hfbsbuf(nbuf)} {incr i} {
	$lbox insert end [HFBSBufFormatRec $i]
    }

    bind $lbox <Double-ButtonPress-1> "HFBSBufDisp \[$lbox curselection\]"
    bind $lbox <Return> "HFBSBufDisp \[$lbox curselection\]"
}

proc HFBSBufDisp { recno } {
    global hfbsbufdisp hfbsbuf
    foreach fld $hfbsbuf(flds) {
	set hfbsbufdisp($fld) $hfbsbuf($recno,$fld)
    }
    set num [format "%2d" $recno]
    set hfbsbufdisp(label) "Buffer No: $num"
    set hfbsbufdisp(current) $recno
}

proc HFBSBufFormatRec { recno } {
    global hfbsbuf
    set output [format "%2d " $recno]
    append output [binary format A20 $hfbsbuf($recno,comment)]
    append output [format "%6d "     $hfbsbuf($recno,ncyc)]
    append output [format "%6d "     $hfbsbuf($recno,cyctime)]
    append output [format "%6.2f "   $hfbsbuf($recno,t0)]
    append output [format "%6.2f "   $hfbsbuf($recno,dt)]
    append output [binary format A10 $hfbsbuf($recno,mode)]
    return $output
}

proc HFBSBufUpdate { } {
    global hfbsbufdisp hfbsbuf
    set recno $hfbsbufdisp(current)
    foreach fld $hfbsbuf(flds) {
	set hfbsbuf($recno,$fld) $hfbsbufdisp($fld)
    }
    # update display on the list
    set lbox $hfbsbufdisp(lbox)
    $lbox delete $recno
    $lbox insert $recno [HFBSBufFormatRec $recno]
    # update buffer information on disk
    HFBSBufWriteFile
}

proc HFBSBufEditShow { } {
    global hfbsbuf
    if [winfo exists $hfbsbuf(toplevel)] {
	wm deiconify $hfbsbuf(toplevel)
    }
}

proc HFBSBufEditHide { } {
    global hfbsbuf
    if [winfo exists $hfbsbuf(toplevel)] {
	wm withdraw $hfbsbuf(toplevel)
    }
}

proc HFBSBufWriteFile { } {
    global hfbsbuf

    set f [open hfbsbuf.prm w]
    for {set i 0} {$i < $hfbsbuf(nbuf)} {incr i} {
	foreach fld $hfbsbuf(flds) {
	    switch $fld {
		comment {
		    set outline "set hfbsbuf($i,$fld) \"$hfbsbuf($i,$fld)\""
		}
		user {
		    set outline "set hfbsbuf($i,$fld) \"$hfbsbuf($i,$fld)\""
		}
		default {
		    set outline "set hfbsbuf($i,$fld) $hfbsbuf($i,$fld)"
		}
	    }
	    puts $f $outline
	}
    }
    close $f
}

proc HFBSBufReadFile { } {
    if {![file exists hfbsbuf.prm]} {
	return -code error "No parameter file"
    }
    uplevel #0 source hfbsbuf.prm
}

###############################################################################
# GUI helper functions
#

proc About { } {
  tk_dialog .about {About...} \
    "Prepare\nN.C. Maliszewskyj\n<nickm@nist.gov>" \
    info 0 OK
}

proc ScrolledListbox { parent args } {
    # Create listbox attached to scrollbars, pass through $args
    frame $parent
    eval {listbox $parent.list \
	    -yscrollcommand [list $parent.sy set] -takefocus 0 } $args
    # Create scrollbar attached to the listbox
    scrollbar $parent.sy -orient vertical \
	    -command [list $parent.list yview]
    pack $parent.sy -side right -fill y
    # pack to allow for resizing
    pack $parent.list -side left -fill both -expand true
    return $parent.list
}

proc ScrolledLabelledListbox { parent {title ""} args } {
    global tcl_platform
    set fontname fixed
    if [regexp {windows} $tcl_platform(platform)] {
	set fontname ansifixed
    }

    # Create listbox attached to scrollbars, pass through $args
    frame $parent
    eval {listbox $parent.list -font $fontname -takefocus 0 \
	    -yscrollcommand [list $parent.sy set] } $args
    # Create scrollbar attached to the listbox
    scrollbar $parent.sy -orient vertical \
	    -command [list $parent.list yview]
    label $parent.title -text $title -borderwidth 4 -anchor w -font $fontname
    
    pack $parent.title -expand true -fill x
    pack $parent.sy -side right -fill y
    pack $parent.list -side left -fill both -expand true
    # pack to allow for resizing
    return $parent.list
}


###############################################################################
#
#
option add *Notebook.borderWidth 2 widgetDefault
option add *Notebook.relief sunken widgetDefault

proc notebook_create { win } {
    global nbInfo

    frame $win -class Notebook

    pack propagate $win 0

    set nbInfo($win-count)   0
    set nbInfo($win-pages)   ""
    set nbInfo($win-current) ""

    # Remember to bind to destroy method
    bind $win <Destroy> "notebook_destroy $win"

    return $win
}

proc notebook_destroy { win } {
    global nbInfo

    foreach p [array names nbInfo] {
	if [regexp "^$win" $p] {
	    unset nbInfo($p)
	}
    }
}

proc notebook_page { win name } {
    global nbInfo

    set page "$win.page[incr nbInfo($win-count)]"
    lappend nbInfo($win-pages)  $page
    set nbInfo($win-page-$name) $page

    frame $page

    if {$nbInfo($win-count) == 1} {
	after idle [list notebook_display $win $name]
    }
    
    return $page
}

proc notebook_display {win name} {
    global nbInfo

    set page ""
    if {[info exists nbInfo($win-page-$name)]} {
	set page $nbInfo($win-page-$name)
    } elseif {[winfo exists $win.page$name]} {
	set page $win.page$name
    }
    if {"" == $page} {
	error "bad notebook page \"$name\""
    }
    
    # perform size calculation
    notebook_fix_size $win
    if {"" != $nbInfo($win-current)} {
	pack forget $nbInfo($win-current)
    }
    pack $page -expand yes -fill both
    set nbInfo($win-current) $page
    return $page
}

proc notebook_fix_size { win } {
    global nbInfo

    update idletasks

    set maxw 0
    set maxh 0
    foreach page $nbInfo($win-pages) {
	set w [winfo reqwidth $page]
	if {$w > $maxw} {set maxw $w}

	set h [winfo reqheight $page] 
	if {$h > $maxh} {set maxh $h} 
    }

    set bd [$win cget -borderwidth]
    set maxw [expr $maxw + 2 * $bd]
    set maxh [expr $maxh + 2 * $bd]
    $win configure -width $maxw -height $maxh
}

###############################################################################
# Build GUI
#

proc HFBSBufEditBuild { } {
    set hfbsbuf(toplevel) [toplevel .runprep ]
    wm title .runprep "Run Setup"
    wm withdraw .runprep

    ScrolledLabelledListbox .runprep.l \
	    "No Comment               Ncyc CycTim     T0     Dt Mode" \
	    -height 5 -width 50
    set hfbsbufdisp(lbox) .runprep.l.list
    notebook_create .runprep.e
    set page [notebook_page .runprep.e hbuf ]
    HFBSBufEntryCreate $page
    notebook_display .runprep.e hbuf

    pack .runprep.l -expand true -fill x -in .runprep
    pack .runprep.e -side top            -in .runprep 
    return .runprep
}

###############################################################################
# Final Initializations
#
wm withdraw .
#HFBSBufEditBuild
#HFBSBufList
#HFBSBufDisp 0
#HFBSBufEditShow