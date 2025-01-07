#!/usr/bin/perl

sub scan_insn_using_r2 (@) {
    my $insns_ref = @_[0];
    my $i, $num_insns, $danger_p;

    $danger_p = 0;
    $num_insns = @$insns_ref;
    for ($i = 0; $i < $num_insns; ++$i) {
	if ($insns_ref->[$i] =~ m/\(r2\)/) {
	    $insns_ref->[$i] = "NG" . $insns_ref->[$i];
	    $danger_p = 1;
	} else {
	    $insns_ref->[$i] = "  " . $insns_ref->[$i];
	}

    }
    return $danger_p;
}

sub filter ($) {
    my $objfile = $_[0];
    my $indirect_call = 0;
    my $line;
    my @call_seq;
    my $ppu_obj_p = 0;
    my $funname_printed_p = 0;
    my $obj_filename = 0;

    # check ELF file format
    unless(open (RE, "ppu-lv2-readelf -h $objfile |")) {
	printf STDERR "ERROR: cannot opne pipe ppu-lv2-readelf";
	exit -1;
    }
    while (<RE>) {
	if (m/CellOS Lv-2/) {
	    $ppu_obj_p = 1;
	    last;
	}
    }
    close (RE);
    # If the object file is not for PPU, skip it.
    if ($ppu_obj_p == 0) {
	printf "filename: $objfile. it is not PPU object\n";
	return;
    }

    unless (open (OD, "ppu-lv2-objdump -d --demangle $objfile |")) {
	printf STDERR "ERROR: cannot opne pipe ppu-lv2-objdump";
	return;
    }

    printf ("filename: $objfile\n");

    while ($line = <OD>) {

	if ($line =~ m/^([^:]+):\s+file format (.*)/) {
	    $obj_filename = $1;
	    $target = $2;

	    unless ($target =~ m/celloslv2/) {
		close (OD);
		return;
	    }
	    
	}
		
	if ($line =~ m/^[0-9a-f]+ (.*)/) {
	    $funname = $1;
	    $funname =~ s/^<\.//;
	    $funname =~ s/>:$//;
	    next;
	}

	next unless ($line =~ m/\s+[0-9a-f]{2} [0-9a-f]{2} [0-9a-f]{2} [0-9a-f]{2}\s+([a-z]\w+.*)/ );
	my $insn = $1;
	$insn =~ s/\# [0-9a-f]+$//;
	$insn =~ s/\s+$//;
	($mnemo, @operands) = split (/[,\s\(\)]+/, $insn);

	if (0) {
	    print "\t" . $mnemo . "\t";
	    foreach $i (@operands) {
		print $i . " ";
	    }
	    print "\n";
	}

	if ($operands[0] eq "r2"
	    && $mnemo eq "lwz")
	{
	    # following part are not suspcious. So skip it.
	    # lwz r2, ...
	    # ....
	    # lwz r2, ...
	    # FIXME: branch optimizer may generate following sequence.
	    # this script cannot picks up this pattern.
            #    std    r2,40(r1)
	    #    cmp   rX,Y
	    #    be    L1
	    #    lwz   r10,0(r20)
	    #    lwz   r11,4(r20)
            #    lfs   f1,zzz(r2)
            #    mtctr r11
            #    b     L2
            # L1:
	    #    lwz   r12,0(r21)
	    #    lwz   r13,4(r21)
            #    lfs   f1,www(r2)
            #    mtctr r13 
            # L2:
            #    bctrl
            #    ld   r2,40(r1)
	    if ($indirect_call == 1) {
		@call_seq = ();
	    }
	    $indirect_call = 1;
	    push (@call_seq, $line);
	    next;
	}
	elsif ($indirect_call == 1
	       && $mnemo eq "bctrl")
	{
	    $indirect_call = 0;
	    push (@call_seq, $line);

	    if (scan_insn_using_r2(\@call_seq) == 1)
	    {
		if ($funname_printed == 0) {
		    print "    object: $obj_filename\n";
		    print "    function: $funname\n";
		    $funname_printed = 1;
		}

		print ("\t--- compiler bug ---\n");
		foreach $di (@call_seq) {
		    print "\t" . $di;
		}
	    }
	    @call_seq = ();
	    next;
	}
	elsif ($indirect_call == 1)
	{
	    push (@call_seq, $line);
	}

    }

    close (OD);
}

foreach $i (@ARGV) {
    filter ($i);
}
exit (0);
