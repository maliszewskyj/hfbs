#!/usr/bin/wish

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


proc SequenceRead { } {
    global env sequence

    if {![file exists $sequence(file)]} {
	return
    }

    if [catch {open $sequence(file) r} sf] {
	return 
    }
    
    set contents [read $sf]
    $sequence(list) delete 0 end
    foreach s [split $contents \n] {
	$sequence(list) insert end $s
    }
    close $sf
}

proc SequenceWrite { } {
    global env sequence

    set sf [open $sequence(file) w]
    set contents [join [$sequence(list) get 0 end] "\n"]
    puts -nonewline $sf $contents
    close $sf
}

proc SequenceFind { } {
    # Find the process ID of the core program
    if [catch {exec ps ax | grep sequence | grep tclsh} result] {
	return 0
    }
    if {![regexp {^[ ]*([0-9]+)} $result match pid]} {
	#Can't determine process ID of sequence program
	return -code error -1
    }
    return $pid
}

proc SequenceStart { } {
    global env sequence
    if [catch SequenceFind pid] {
	return -code error "Can't determine sequence PID"
    }

    if {$pid > 0} {
	set answer [tk_messageBox -type yesno -message \
		"Sequence in progress: Cancel?"]
	switch $answer {
	    yes { 
		if [catch {SequenceKill $pid} result] {
		    return -code error $result
		}
	    }
	    no {
		return
	    }
	}
    }

    # Start sequence
    if [catch {exec /usr/local/bin/sequence $sequence(file) &} result] {
	puts stderr $result
    }

}

# Kill a sequence currently in progress
#
proc SequenceKill { args } {
    global env sequence
    if { [llength $args] > 0 } {
	set pid [lindex $args 0]
    } else {
	if [catch SequenceFind pid] {
	    return -code error "Can't determine sequence PID"
	}
    }
    
    if [catch {exec kill -s 9 $pid} result] {
	return -code error "Can't kill sequence process $pid"
    }

    # Should determine whether a run is in progress and kill it, too
    
}

proc SequenceEditorBuild { parent } {
    global env sequence

    # initialize array sequence
    set sequence(run) 0
    set sequence(wait) 1
    set sequence(temp) 300.0
    set sequence(dop)  10.0
    set sequence(file) $env(HOME)/hfbs.seq
    set sequence(toplevel) [toplevel $parent]
    wm withdraw $sequence(toplevel)

    set tf [frame $parent.top]
    set sequence(list) [ScrolledListbox $tf.l -width 20 -height 8]

    set f [frame $tf.f1 -bd 2 -relief ridge]
    set frl [label $f.runlab    -text Run]
    set ftl [label $f.templab   -text Temperature]
    set fwl [label $f.waitlab   -text Wait]
    set fdl [label $f.doplab    -text Doppler]
    set fru [label $f.rununits  -text (Buffer)]
    set ftu [label $f.tempunits -text (K)]
    set fwu [label $f.waitunits -text (Min)]
    set fdu [label $f.dopunits  -text (Hz)]
    set fre [entry $f.runentry  -width 10 -textvariable sequence(run)]
    set fte [entry $f.tempentry -width 10 -textvariable sequence(temp)]
    set fwe [entry $f.waitentry -width 10 -textvariable sequence(wait)]
    set fde [entry $f.dopentry  -width 10 -textvariable sequence(dop)]
    set fri [button $f.runbut   -text Insert -command SequenceInsertRun]
    set fti [button $f.tempbut  -text Insert -command SequenceInsertTemp]
    set fwi [button $f.waitbut  -text Insert -command SequenceInsertWait]
    set fdi [button $f.dopbut   -text Insert -command SequenceInsertDop]

    grid $frl $fru $fre $fri -in $f -sticky w
    grid $ftl $ftu $fte $fti -in $f -sticky w
    grid $fwl $fwu $fwe $fwi -in $f -sticky w
    grid $fdl $fdu $fde $fdi -in $f -sticky w

    set bf  [frame $parent.bot]
    set fd  [button $bf.delete   -text Delete  -width 8 \
	    -command SequenceDeleteElement]
    set fc  [button $bf.clear    -text Clear   -width 8 \
	    -command SequenceClear]
    set fa  [button $bf.apply    -text Apply   -width 8 \
	    -command SequenceWrite]
    set fx  [button $bf.dismiss  -text Dismiss -width 8 \
	    -command SequenceEditorHide]

    pack $fd $fc $fa $fx -side left -in $bf -expand true -fill x
    pack $tf.l $f -side left -in $tf
    pack $tf $bf -side top -in $sequence(toplevel) 
    return $sequence(toplevel)
}

proc SequenceClear { } {
    global sequence
    $sequence(list) delete 0 end
}

proc SequenceDeleteElement { } {
    global sequence
    set idx [$sequence(list) curselection]
    if {[string length $idx] == 0} {set idx end}
    $sequence(list) delete $idx
}

proc SequenceInsertRun { } {
    global sequence
    set cmd "run buffer $sequence(run)"
    set idx [$sequence(list) curselection]
    if {[string length $idx] == 0} {set idx end}
    $sequence(list) insert $idx $cmd
}

proc SequenceInsertTemp { } {
    global sequence
    set cmd "temperature set $sequence(temp)"
    set idx [$sequence(list) curselection]
    if {[string length $idx] == 0} {set idx end}
    $sequence(list) insert $idx $cmd
}

proc SequenceInsertWait { } {
    global sequence
    set cmd "wait minutes $sequence(wait)"
    set idx [$sequence(list) curselection]
    if {[string length $idx] == 0} {set idx end}
    $sequence(list) insert $idx $cmd
}

proc SequenceInsertDop { } {
    global sequence
    set cmd "doppler speed $sequence(dop)"
    set idx [$sequence(list) curselection]
    if {[string length $idx] == 0} {set idx end}
    $sequence(list) insert $idx $cmd
}

proc SequenceEditorShow { } {
    global sequence
    SequenceRead
    wm deiconify $sequence(toplevel)
}

proc SequenceEditorHide { } {
    global sequence
    wm withdraw $sequence(toplevel)
}

#
#
#
#
wm withdraw .
SequenceEditorBuild .sequence
SequenceEditorShow
