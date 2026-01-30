#!/usr/bin/wish
#
# TODO - read and write from data file
#
#      - bind entry mask fields
#
#
#
#
set tcl_precision 4
set ibuf_fields { a1beg a1end a1inc a2beg a2end a2inc a3beg a3end a3inc \
                  a4beg a4end a4inc a5beg a5end a5inc a6beg a6end a6inc \
                  comment ter th th0 mon mt mpf t0 it tw pts \
	          flip1 flip2 flip3 flip4 hf ihf hfw hfh }

set qbuf_fields { comment a b c aa bb cc ec es ef eft \
	          hc kc lc hs ks ls pts hkl11 hkl12 hkl13 hkl21 hkl22 hkl23 \
                  tmp it tw th0 ter hf mon mpf mt flip1 flip2 flip3 flip4 \
		  ihf hfw hfh th }


proc IBufEntryCreate { parent } {
    global icpdisp
    label $parent.bufnolabel -textvariable icpdisp(label)
    grid $parent.bufnolabel -row 0 -column 0
    label $parent.commentlabel -text "Comment:"
    grid $parent.commentlabel -row 0 -column 1 
    entry $parent.comment -width 35 -textvariable icpdisp(comment)
    grid $parent.comment -row 0 -column 2 -columnspan 6 -sticky ew
    for {set i 1} {$i <= 6} {incr i} {
	label $parent.a${i}beglabel -text "A${i}-beg:"
	grid $parent.a${i}beglabel -row $i -column 1
	entry $parent.a${i}beg -width 7 -textvariable icpdisp(a${i}beg)
	grid $parent.a${i}beg -row $i -column 2
	label $parent.a${i}inclabel -text "Inc-A${i}:"
	grid $parent.a${i}inclabel -row $i -column 3
	entry $parent.a${i}inc -width 7 -textvariable icpdisp(a${i}inc)
	grid $parent.a${i}inc -row $i -column 4
	label $parent.a${i}endlabel -text "A${i}-end:"
	grid $parent.a${i}endlabel -row $i -column 5
	entry $parent.a${i}end -width 7 -textvariable icpdisp(a${i}end)
	grid $parent.a${i}end -row $i -column 6
	bind $parent.a${i}beg <Return>   "IBufAngleBinding $i"
	bind $parent.a${i}inc <Return>   "IBufAngleBinding $i"
	bind $parent.a${i}beg <FocusOut> "IBufAngleBinding $i"
	bind $parent.a${i}inc <FocusOut> "IBufAngleBinding $i"
    }
    label $parent.tlabel -text "Temp/H:"
    grid $parent.tlabel -row 7 -column 0
    frame $parent.t
    grid $parent.t -row 7 -column 1 -columnspan 6
    label $parent.t.t0label -text "T0:"
    pack $parent.t.t0label -in $parent.t -side left
    entry $parent.t.t0 -width 6 -textvariable icpdisp(t0)
    pack $parent.t.t0 -in $parent.t -side left
    label $parent.t.itlabel -text "Inc-T:"
    pack $parent.t.itlabel -in $parent.t -side left
    entry $parent.t.it -width 6 -textvariable icpdisp(it)
    pack $parent.t.it -in $parent.t -side left
    label $parent.t.twlabel -text "Wait:"
    pack $parent.t.twlabel -in $parent.t -side left
    entry $parent.t.tw -width 3 -textvariable icpdisp(tw)
    pack $parent.t.tw -in $parent.t -side left
    label $parent.t.errlabel -text "Err:"
    pack $parent.t.errlabel -in $parent.t -side left
    entry $parent.t.err -width 3 -textvariable icpdisp(err)
    pack $parent.t.err -in $parent.t -side left
    label $parent.t.hld0label -text "Hld0:"
    pack $parent.t.hld0label -in $parent.t -side left
    entry $parent.t.hld0 -width 3 -textvariable icpdisp(th0)
    pack $parent.t.hld0 -in $parent.t -side left
    label $parent.t.hldlabel -text "Hld:"
    pack $parent.t.hldlabel -in $parent.t -side left
    entry $parent.t.hld -width 3 -textvariable icpdisp(th)
    pack $parent.t.hld -in $parent.t -side left
    button $parent.t.field -text FIELD -command IBufFieldDialog
    pack $parent.t.field -in $parent.t -side left
    label $parent.timelabel -text "Time:"
    grid $parent.timelabel -in $parent -row 8 -column 0
    grid [frame $parent.time] -in $parent -row 8 -column 1 -columnspan 6
    label $parent.time.monlabel -text "Monit:"
    pack $parent.time.monlabel -in $parent.time -side left
    entry $parent.time.mon -width 6 -textvariable icpdisp(mon)
    pack $parent.time.mon -in $parent.time -side left
    label $parent.time.prelabel -text "Prefac:"
    pack $parent.time.prelabel -in $parent.time -side left
    entry $parent.time.pre -width 3 -textvariable icpdisp(mpf)
    pack $parent.time.pre -in $parent.time -side left
    label $parent.time.mtlabel -text "M-typ:"
    pack $parent.time.mtlabel -in $parent.time -side left
    radiobutton $parent.time.mtt -text time -variable icpdisp(mt) -value time
    radiobutton $parent.time.mtn -text neut -variable icpdisp(mt) -value neut
    button $parent.time.pol -text "POLARIZE" -command IBufPolDialog
    pack $parent.time.mtt $parent.time.mtn $parent.time.pol \
	    -in $parent.time -side left

    frame $parent.pts
    label $parent.pts.label -text "NPTS:"
    entry $parent.pts.val -textvariable icpdisp(pts) -width 6
    pack $parent.pts.label $parent.pts.val -in $parent.pts -side left
    grid $parent.pts -row 1 -column 7
    bind $parent.pts.val <Return>   IBufAngleBinding
    bind $parent.pts.val <FocusOut> IBufAngleBinding

    return $parent
}

proc IBufReadRec { file rec } {
    upvar $rec r
    set input [read $file 320]
    binary scan $input \
	    A50sfffffffffffffffffffffA4ccccffff \
	    r(comment) pad1 \
	    r(a1beg) r(a2beg) r(a3beg) r(a4beg) r(a5beg) r(a6beg) \
	    r(a1inc) r(a2inc) r(a3inc) r(a4inc) r(a5inc) r(a6inc) \
	    r(pts) r(hf) r(t0) r(it) r(tw) r(th0) r(ter) \
	    r(mon) r(mpf) r(mt) \
	    r(flip1) r(flip2) r(flip3) r(flip4) \
	    r(ihf) r(hfw) r(hfh) r(th)
}

proc IBufReadFile { } {
    global icpbuf ibuf_fields

    set f [open IBUFFER.BUF r]
    for {set i 0} {$i < 30} {incr i} {
	IBufReadRec $f icpin
	foreach fld $ibuf_fields {
	    if {![catch {set icpin($fld)} result]} {
		set icpbuf($i,$fld) $result
	    }
	}
    }
    close $f
}
 
proc IBufWriteRec { file rec } {
    upvar $rec r

    set pad1 0
    set recval [binary format \
	    A50sfffffffffffffffffffffA4ccccffff \
	    $r(comment) $pad1 \
	    $r(a1beg) $r(a2beg) $r(a3beg) $r(a4beg) $r(a5beg) $r(a6beg) \
	    $r(a1inc) $r(a2inc) $r(a3inc) $r(a4inc) $r(a5inc) $r(a6inc) \
	    $r(pts) $r(hf) $r(t0) $r(it) $r(tw) $r(th0) $r(ter) \
	    $r(mon) $r(mpf) $r(mt) \
	    $r(flip1) $r(flip2) $r(flip3) $r(flip4) \
	    $r(ihf) $r(hfw) $r(hfh) $r(th)]
    set record [binary format a320 $recval]
    puts -nonewline $file $record
    string length $record
}


proc IBufWriteFile { } {
    global icpbuf ibuf_fields

    set f [open IBUFFER.BUF w]
    for {set i 0} {$i < 30} {incr i} {
	foreach fld $ibuf_fields {
	    set icpout($fld) $icpbuf($i,$fld)
	}
	IBufWriteRec $f icpout
    }
    puts -nonewline $f [binary format a320 "\#"]
    close $f
}

proc IBufInit { } {
    global icpbuf 
    array set icpbuf {buftype Increment}
    for {set i 0} {$i < 30} {incr i} {
	set icpbuf($i,comment) "empty"
	for {set j 1} {$j <= 6} {incr j} {
	    set icpbuf($i,a${j}beg)   0.0
	    set icpbuf($i,a${j}inc)   0.0
	    set icpbuf($i,a${j}end)   0.0
	}
	set icpbuf($i,t0)    0
	set icpbuf($i,it)    0
	set icpbuf($i,tw)    0
	set icpbuf($i,ter)   0
	set icpbuf($i,th0)   0
	set icpbuf($i,th)    0
	set icpbuf($i,mon)   1000
	set icpbuf($i,mpf)   1
	set icpbuf($i,mt)    time
	set icpbuf($i,pts)   0
	set icpbuf($i,flip1) 0
	set icpbuf($i,flip2) 0
	set icpbuf($i,flip3) 0
	set icpbuf($i,flip4) 0
	set icpbuf($i,hf)    0
	set icpbuf($i,ihf)   0
	set icpbuf($i,hfw)   0
	set icpbuf($i,hfh)   0
    }
    set icpbuf(currentrec)   0
    IBufDisp 0
}

proc IBufList { } {
    # Clear out our list if it contains anything.
    set lbox .l.list
    $lbox delete 0 end
    $lbox configure -font fixed
    for {set i 0} {$i < 30} {incr i} {
	$lbox insert end [IBufFormatRec $i]
    }

    bind $lbox <Double-ButtonPress-1> "IBufDisp \[$lbox curselection\]"
    bind $lbox <Return> "IBufDisp \[$lbox curselection\]"
}

proc IBufFormatRec { recno } {
    global icpbuf
    set output [format "%2d " $recno]
    append output [binary format A20 $icpbuf($recno,comment)]
    append output [format " %6.2f " $icpbuf($recno,a4beg)]
    append output [format "%6.2f " $icpbuf($recno,a4end)]
    append output [format "%5.2f " $icpbuf($recno,a4inc)]
    append output [format "%2.0f\* " $icpbuf($recno,mpf)]
    append output [format "%6.0f "   $icpbuf($recno,mon)]
    return $output
}

proc IBufDisp { recno } {
    global icpdisp icpbuf ibuf_fields
    foreach fld $ibuf_fields {
	set icpdisp($fld) $icpbuf($recno,$fld)
    }
    set num [format "%2d" $recno]
    set icpdisp(label) "IBuffer No: $num"
    set icpdisp(current) $recno
    if [catch {expr $icpdisp(pts) * 1} icpdisp(pts)] {
	set icpdisp(pts) 1
    }
    IBufAngleBinding
}

proc IBufUpdate { } {
    global icpdisp icpbuf 
    set recno $icpdisp(current)
    foreach fld [array names icpdisp] {
	set icpbuf($recno,$fld) $icpdisp($fld)
    }
    # update display on the list
    .l.list delete $recno
    .l.list insert $recno [IBufFormatRec $recno]
    # update buffer information on disk
    IBufWriteFile
}

proc IBufFieldDialog { } {
    global icpdisp icpbuf

    toplevel .field

    label .field.title -text "MAGNETIC FIELD"
    label .field.initlab -text "Initial Field (user units):"
    label .field.incrlab -text "Field Increment (user units):"
    label .field.waitlab -text "Max Wait Per Point (secs) :"
    entry .field.initentry -textvariable icpdisp(hf)  -width 10
    entry .field.increntry -textvariable icpdisp(ihf) -width 10
    entry .field.waitentry -textvariable icpdisp(ihw) -width 10
    label .field.t1 -text "(Wait used in non-persistent mode on SC only)"
    label .field.t2 -text "(Reply 0 to get default optimum wait times)"
    button .field.dismiss -text "Dismiss" -command IBufFieldDialogDismiss
    grid .field.title     -row 0 -column 0 -columnspan 2 -in .field \
	    -sticky news
    grid .field.initlab   -row 1 -column 0 -in .field
    grid .field.incrlab   -row 2 -column 0 -in .field
    grid .field.waitlab   -row 3 -column 0 -in .field
    grid .field.initentry -row 1 -column 1 -in .field
    grid .field.increntry -row 2 -column 1 -in .field
    grid .field.waitentry -row 3 -column 1 -in .field
    grid .field.t1        -row 4 -column 0 -columnspan 2 -sticky ew
    grid .field.t2        -row 5 -column 0 -columnspan 2 -sticky ew
    grid .field.dismiss   -row 6 -column 0 -columnspan 2 -sticky ew
    return .field
}

proc IBufFieldDialogDismiss { } {
    destroy .field
}

proc IBufPolDialog { } {
    global icpdisp
    set p [toplevel .polarization]
    label $p.title -text "Polarization Setup"
    set f [frame $p.f]
    set d [button $p.dismiss -text "Dismiss" -command IBufPolDialogDismiss]
    pack $p.title $f $d -side top -fill x
    label $f.flip1t -text "Flipper 1" -relief ridge 
    label $f.flip2t -text "Flipper 2" -relief ridge 
    label $f.flipet -text "Execute ?" -relief ridge 
    frame $f.flip1 -borderwidth 2 -relief ridge 
    frame $f.flip2 -borderwidth 2 -relief ridge 
    frame $f.flipe -borderwidth 2 -relief ridge 
    grid $f.flip1t $f.flip2t $f.flipet -in $f -sticky ew
    grid $f.flip1 $f.flip2 $f.flipe -in $f -sticky ew
    label $f.flip1.a -text "OFF" -pady 5
    label $f.flip1.b -text "ON"  -pady 5
    label $f.flip1.c -text "OFF" -pady 5
    label $f.flip1.d -text "ON"  -pady 5
    pack $f.flip1.a $f.flip1.b $f.flip1.c $f.flip1.d -side top
    label $f.flip2.a -text "OFF" -pady 5
    label $f.flip2.b -text "OFF" -pady 5
    label $f.flip2.c -text "ON"  -pady 5
    label $f.flip2.d -text "ON"  -pady 5
    pack $f.flip2.a $f.flip2.b $f.flip2.c $f.flip2.d -side top
    toggle_create $f.flipe.a icpdisp(flip1)
    toggle_create $f.flipe.b icpdisp(flip2)
    toggle_create $f.flipe.c icpdisp(flip3)
    toggle_create $f.flipe.d icpdisp(flip4)
    pack $f.flipe.a $f.flipe.b $f.flipe.c $f.flipe.d -side top -fill x
    return $p
}

proc IBufPolDialogDismiss { } {
    destroy .polarization
}

proc IBufAngleBinding {{anglist {1 2 3 4 5 6}}} {
    global icpdisp
    if [catch {expr $icpdisp(pts) * 1} icpdisp(pts)] {
	set icpdisp(pts) "******"
	return
    }
    
    foreach i $anglist {
	set beg "a${i}beg"
	set inc "a${i}inc"
	set end "a${i}end"
	if [catch {expr $icpdisp($beg) * 1.0} icpdisp($beg)] {
	    set icpdisp($beg) "******"
	    return
	}
	if [catch {expr $icpdisp($inc) * 1.0} icpdisp($inc)] {
	    set icpdisp($inc) "******"
	    return
	}
	set icpdisp($end) [expr $icpdisp($beg) + \
		$icpdisp($inc) * ($icpdisp(pts) - 1)]
    }
    update
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
	    -yscrollcommand [list $parent.sy set] } $args
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
    eval {listbox $parent.list -font $fontname \
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

proc toggle_create { win boolvar {boolvals {OFF ON}}} {
    global tInfo

    upvar $boolvar b
    if {[llength $boolvals] != 2} {
	return -code error "toggle_create: specify only two possible labels"
    }

    if {$b} {
	set initvar [lindex $boolvals 1]
    } else {
	set initvar [lindex $boolvals 0]
    }
    set tInfo($win-var) $boolvar
    set tInfo($win-lab) $boolvals
    button $win -text $initvar -command "toggle_action $win"

    bind $win <Destroy> "toggle_destroy $win"

    return $win
}

proc toggle_destroy { win } {
    global tInfo
    unset tInfo($win-var)
    unset tInfo($win-lab)
}

proc toggle_action { win } {
    global tInfo
    upvar $tInfo($win-var) t
    if {$t} {
	$win configure -text [lindex $tInfo($win-lab) 0]
	set t 0
    } else {
	$win configure -text [lindex $tInfo($win-lab) 1]
	set t 1
    }
    return $t
}

IBufInit
###############################################################################
# Build GUI
#
wm title . "ICP Prepare"
set m [frame .mbar -relief raised -borderwidth 2]
set mf [menubutton $m.file    -width 5 -text File    -underline 0 \
	-menu $m.file.menu]
set mfm [menu $mf.menu]
$mfm add command -label "About" -underline 0 -command About
$mfm add command -label "Exit"  -underline 1 -command exit

#ScrolledListbox .l -width 60 -height 6
ScrolledLabelledListbox .l \
	"No Comment *I-Buffer*   A4-beg A4-end  Inc    Monitor " \
	-height 6 -width 80
notebook_create .e
set page [notebook_page .e ibuf ]
IBufEntryCreate $page
set page [notebook_page .e msg  ]
label $page.label -text "No messages"
pack $page.label
set page [notebook_page .e blank]
notebook_display .e ibuf

# Pack widgets
frame .k
label .k.f9  -text "Update: F9"
label .k.f10 -text "BufOps: F10"
label .k.f11 -text "Reveal: F11"
label .k.f12 -text "Hide: F12"

bind all <KeyPress-F9>  IBufUpdate
bind all <KeyPress-F10> {puts "BUFFER OPS"}
bind all <KeyPress-F11> {notebook_display .e ibuf}
bind all <KeyPress-F12> {notebook_display .e blank}

pack $m -side top -fill x -anchor nw
pack $mf -side left -in $m
pack .l -expand true -fill x
pack .e -side top
pack .k -expand true -fill x
pack .k.f9 .k.f10 .k.f11 .k.f12 -in .k -side left

# Last minute initializations
#
#

if {[file exists IBUFFER.BUF]} {
    # If the buffer file exists, read
    IBufReadFile
} else {
    # Write the contents of icpbuf to IBUFFER.BUF
    IBufWriteFile
}
IBufList 
IBufDisp 0
