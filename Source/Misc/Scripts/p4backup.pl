#!/usr/bin/perl -w
($sec,$min,$hour,$day, $month, $year) = (localtime)[0..5];
$year=$year+1900;
$month++;
$datestring=sprintf("%d-%02s-%02s\_%02s-%02s-%02s",$year,$month,$day,$hour,$min,$sec);
$tarfile="P4Backup_".$datestring.".tar";
@p4Files=split(/\n/,`p4 opened`);

$file=$p4Files[0];
$file =~ s/#.*//;
$file =~ /\/\/depot\/(.+)/;
$match=$1;
$file =(split(/ /,`p4 where $file`))[2];
$file =~ s/\\/\//g;
$file =~ /(.+)$match/;
chdir("$1");
$depotdir=$1;
$tarcmd="tar \-cf ".$tarfile;

print "Backing up files...\n";

foreach $file (@p4Files)
{
	$file =~ s/#.*//;
	$file =~ s/\/\/depot\//\.\//;

	$file =`cygpath \"$file\"`;
	$file =~ s/\s+//g;
	$tarcmd=$tarcmd." $file";
	print $file."\n";
}
print `$tarcmd`;
print `gzip $tarfile`;
print "\nFiles archived in \"$depotdir$tarfile.gz\"\n"

