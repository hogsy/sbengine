#!/bin/perl
my @ps=`ps`;
my $distcccmd='/usr/bin/distccd';
my $cnt=0;
foreach my $proc(@ps) {
	if ($proc=~/..(.{8}).{47}($distcccmd)/)
	{
		`kill -9 $1`;
		$cnt++;
	}
}
print "Killed $cnt instances of distccd\n";
