#print STDOUT $ARGV[0]."\na\n";
#foreach $arg(@ARGV) {
#	print STDOUT $i."  ".$arg."\n";
#	$i++;
#}
@lines=split(/[\n\r\f]/,$ARGV[1]);
$i=0;
foreach $line(@lines) {
	next if ($line eq "");
	next if ($line=~/ERROR: failed to write: Connection reset by peer/);
	$line=~s/\/\//\//g;
	$line=~s/\:([0-9]+)\:([0-9]+)*[\:]*/($1):/g;
	$line=~s/error: #error/error:/g;
	$line=~/(([A-Za-z]\:\/)*([\w\.\-]+\/)+[\w\.\-]+)/;
	my $prjDir=pathMerge($ARGV[0],$1);
	$prjdir=~s/\//\\/g;
	$line=~s/(([A-Za-z]\:\/)*([\w\.\-]+\/)+[\w\.\-]+)/$prjDir/;
	print STDOUT $line."\n";
	$i++;
}
#print "\n\n";
#print STDOUT $ARGV[1]."\n\n";


sub pathMerge {
	my $p0=shift;
	my $p1=shift;
	my @p0L=split(/\//,$p0);
	my @p1L=split(/\//,$p1);
	return $p1 if (($p1L[0]=~/^\w\:/) && ($p0L[0]=~/^\w\:/));
	shift(@p1L) if ($p1L[0]=~/^\.$/);
	shift(@p0L) if ($p0L[0]=~/^\.$/);
	
	$cnt=0;
	while (($p1L[0]=~/\.\./) && (scalar(@p0L)>0)) {
		pop(@p0L);
		shift(@p1L);
	}
	while ($p0L[$cnt] eq $p1L[0]) {
		shift(@p1L);
		$cnt++;
	}
	my $newPath="";
	foreach my $p(@p0L) {
		$newPath=$newPath."\/".$p;
	}
	foreach my $p(@p1L) {
		$newPath=$newPath."\/".$p;
	}
	$newPath=~s/^\///;
	return $newPath;
}
