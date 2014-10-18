#!/usr/bin/perl

# Usage: bake.pl sernum < inelf > outelf

while (read STDIN, $x, 16) {
	if ($x eq "XXXXXXXXXXXXXXX\0") {
		my $zerolen = 16 - (length $ARGV[0]);
		print "$ARGV[0]" . "\0"x$zerolen;
	} else {
		print $x;
	}
}
