#!/usr/bin/tclsh
# $Id: rpc.tcl,v 1.2 2008/05/08 13:11:51 nickm Exp $
proc ldelete { list value } {
    set ix [lsearch -exact $list $value]
    if {$ix >= 0} {
	return [lreplace $list $ix $ix]
    } else {
	return $list
    }
}

proc RPCHandler { sock } {
    global comm errorInfo errorCode

    if {[eof $sock] || [catch {gets $sock line}]} {
	# end of file or abnormal connection drop
	close $sock
	if {$comm(server)} {
	    set comm(clients) [ldelete $comm(clients) $sock]
	}
    } else {
	set comm(current) $sock
	if {[string length $line] && [info complete $line]} {
	    set code [catch {uplevel \#0 $line} result]
	    set reply [list $code $result $errorInfo $errorCode]\n
	    #set reply [list $code $result "" "" ]
	    set lines [regsub -all \n $reply {} junk]
            puts $sock $lines
            puts -nonewline $sock $reply
            flush $sock
	}
    }
}

proc RPCMainHandler { args } {
    global comm

    set sock $comm(main)
    if {[eof $sock] || [catch {gets $sock line}]} {
	# end of file or abnormal connection drop
	close $sock
	exit
    } else {
	if {[string length $line] > 0} {
	    puts $line
	    puts -nonewline $comm(prompt)
	    flush stdout
	}
    }
}

proc RPCDo { args } {
    global comm

    set sock $comm(main)
    # Preserve the concat semantics of eval
    if {[llength $args] > 1} {reply
	set cmd [concat $args]
    } else {
	set cmd [lindex $args 0]
    }
    puts $sock $cmd
    flush $sock

    gets $sock lines
    set result {} 
    while {$lines > 0} { 
	gets $sock x
	append result $x\n
	incr lines -1
    }
    set code [lindex $result 0]
    set x [lindex $result 1]
    # Cleanup the end of the stack
    regsub "\[^\n]+$" [lindex $result 2] \
	    "* Remote Server $comm(main)*" stack
    set ec [lindex $result 3]
    return -code $code -errorinfo $stack -errorcode $ec $x
}

# If the client receives commands from the host
proc RPCRecv { args } {
    global comm
    set sock $comm(main)
    if {[eof $sock] || [catch {gets $sock line}]} {
	# end of file or abnormal connection drop
	close $sock
	puts "Server disconnected!"
	exit
    } else {
	if {[string length $line] && [info complete $line]} {
	    set code [catch {uplevel \#0 $line} result]
	}
    }
}

proc RPCServer { port } {
    global comm
    set comm(server) 1
    if [catch {socket -server RPCAccept $port} result] {
	puts stderr "Error opening server port $port : $result"
	exit
    } 
    set comm(main) $result
}

proc RPCClient { host port } {
    global comm
    set comm(server) 0
    if [catch {socket $host $port} result] {
	return -code error "Error connection to $host:$port"
    }
    set comm(server) [list $host $port]
    set comm(main) $result
    fconfigure $comm(main) -buffering line
    fileevent $comm(main) readable RPCRecv
    return $comm(main)
}

# Accept-Connection handler for Server. 
# called When client makes a connection to the server
# Its passed the channel we're to communicate with the client on, 
# The address of the client and the port we're using
#
# Setup a handler for (incoming) communication on 
# the client channel - send connection Reply and log connection
proc RPCAccept {sock addr port} {
    global comm

    # Setup handler for future communication on client socket
    fileevent $sock readable [list RPCHandler $sock]

    # Note we've accepted a connection (show how get peer info fm socket)
    #puts "Accept from [fconfigure $sock -peername]"

    # Add the new client to the list
    #lappend comm(clients) $sock

    # Read client input in lines, disable blocking I/O
    fconfigure $sock -buffering line -blocking 0

    # Send Acceptance string to client
    #puts $sock "$addr:$port, You are connected to the echo server."

    # log the connection
    # puts "Accepted connection from $addr at [exec date]"
}

