#!/usr/bin/env tclsh

package require PDCurses

pdc::stdscr add AB
pdc::stdscr add {1 2} C
pdc::stdscr add D\n

pdc::stdscr opt keypad on
pdc::stdscr opt delay off

set running 1
while {$running} {
	while {[set input [pdc::stdscr getch]] ne ""} {
		if {$input eq "\n" || $input eq "\33"} {
			set running 0
		} elseif {[string length $input] > 1} {
			pdc::stdscr add "$input "
		}
	}
}
