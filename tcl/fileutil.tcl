#!/usr/bin/tclsh

proc FindFile { startdir namePat FileList } {
    upvar $FileList flist
    set currentdir [pwd]
    if [catch {cd $startdir} err] {
	return -code error "ERR\nDirectory $startdir cannot be accessed"
    }
    foreach match [glob -nocomplain -- $namePat] {
        lappend flist $match
    }

    foreach file [glob -nocomplain *] {
        if [file isdirectory $file] {
            FindFile $startdir/$file $namePat flist
        }
    }
    cd $currentdir
}

proc MakeDirectory { directory } {

   if {![file exists $directory]} {
	if [catch {file mkdir $directory} result] {
	    return -code error $result
	}
    } else {
	# If a file with the directory name exists, delete it and create
	# the directory
	if {![file isdirectory $directory]} {
	    if [catch {file delete $directory} result] {
		return -code error "File $directory cannot be deleted"
	    }
	    if [catch {file mkdir $directory} result] {
		return -code error $result
	    }
	} else {
	    return "Directory $directory exists"
	}
    }

    return "Directory $directory created"
}

proc NewFileName { directory extension } {
#   set stem [string tolower [clock format [clock seconds] -format "%b%d%Y"]_ ]
    set stem [string tolower [clock format [clock seconds] -format "%Y%m%d"]_ ]
    set pattern $stem\(...\)\\.$extension
    FindFile $directory ${stem}*.$extension filelist
    if {![info exists filelist] || [expr [llength $filelist] == 0]} {
	return ${stem}001.$extension
    }
    set filelist [lsort $filelist]
    set lastfile [lindex $filelist [expr [llength $filelist] - 1]]
    if [regexp $pattern $lastfile match number] {
	#scan $number "%d" number
	# Strip leading zero so number will not be interpreted as octal
	regsub {0+([0-9]+)} $number {\1} number
	return ${stem}[format "%03d" [incr number]].$extension
    }
    return -code error "Trouble creating filename"
}
