#!/bin/perl
use Xml::Parser;
use File::Basename;
use Cwd;

$solutionDir=getcwd();
$solutionDir=~s/\/cygdrive\/(\w)/$1\:/;
$solutionDir=~s/^$/\./;
print "Creating mastermakefile.\n";
$solutionDir=~s/\\/\//g;
$slnFilename=$ARGV[0];
$relPath=$ARGV[1];
$relPath=~s/\\/\//g;
$distccHostsFile=$relPath."\/distcchosts.txt";
$makeFilename="Makefile";
$convSettingsFilename=$relPath."\/ConvSettings.txt";
$solutionName=$slnFilename;
$solutionName=~s/\.sln//;

push(@makefile,"\n".'SolutionDir := '.$solutionDir);
push(@makefile,"\n".'SolutionName := '.$solutionName);
push(@makefile,"\n".'SCRIPT_PATH := '.$relPath);
push(@makefile,"\n\n");

parseSettings($convSettingsFilename);

if ($usedistcc) {
	open (F, $distccHostsFile) || die "Can't open distcc hosts file: ".$slnFilename."\n";
	my @File = <F>;
	close (F);
	$distCCHosts='export DISTCC_HOSTS="';
	foreach my $dcc(@File) {
		$dcc=~s/\s//g;
		$distCCHosts=$distCCHosts.$dcc." ";
	}
	$distCCHosts=$distCCHosts.'" && ';
}
if ($useccache) {
	$ccachePrefix='export CCACHE_DIR=c:/ccache && ';
		
	$ccachePrefix=$ccachePrefix.'export CCACHE_PREFIX=$(DISTCC) && ' if ($usedistcc);
	$prefixCommand='$(CCACHE)'
}
else {
	$prefixCommand='$(DISTCC)' if ($usedistcc);
}

open (F, $slnFilename) || die "Can't open solutionfile: ".$slnFilename."\n";
@File = <F>;
close (F);

my $parsingSpuBuild=0;
my $parsingDepFix=0;
my $parsingStub=0;
my $parsingTranslation=0;
my $parsingSkip=0;
for (@File) {
	next if ($_ =~ /^#/);
	$guid='\{[A-F0-9]{8}-[A-F0-9]{4}-[A-F0-9]{4}-[A-F0-9]{4}-[A-F0-9]{12}\}';
	$separator='\s*\,\s*';
	if ($_=~/Project\(\"$guid\"\) = \"(\w+)\"$separator\"(.+)\"$separator\"($guid)/) {
		$parsingProject=1;
		$currentProject=$1;
		my $guid=$3;
		my $tmp=$2;
		$tmp=~s/\\/\//g;
		$H_projDir{$guid}=$tmp;
		$H_projName{$guid}=$currentProject;
	}	
	$parsingProject=0 if ($_=~/EndProject\W/);
	if ($parsingProject) {
		if ($_=~/ProjectSection\s*\(\s*ProjectDependencies\s*\)\s*=\s*postProject/) {
			$parsingDeps=1;
		}
		if ($parsingDeps) {
			$parsingDeps=0 if ($_=~/EndProjectSection\W/);
			if ($_=~/($guid)\s=\s$guid/) {
				push(@{$HoL_deps{$currentProject}},$1);
			}
		}
	}
	$parsingConfigs=1 if ($_=~/GlobalSection\s*\(\s*SolutionConfigurationPlatforms\s*\)\s*\=\s*preSolution/);
	if ($parsingConfigs) {
		if ($_=~/PS3 (.+)\|Win32 \=/) {
			$tmp=$1;
			$tmp=~s/ /_/g;
			push(@L_configs,$tmp);
		}
		$parsingConfigs=0 if ($_=~/	EndGlobalSection\W/);
	}
}


push(@makefile,"\n".'LinkDir := $(SolutionDir)/Output/Ps3/$(Config)');

push(@makefile,"\n\nBuildPrefix := $ccachePrefix $distCCHosts $prefixCommand\n");


push(@makefile,"\nLocalLibs := ");
for my $guid (keys %H_projName) {

	if (exists $HoL_deps{$H_projName{$guid}}) {
		foreach my $dep(@{$HoL_deps{$H_projName{$guid}}}) {
			push(@makefile,"-l$H_projName{$dep}\n");
			push(@makefile,"LocalLibs += ");
		}
	}
}
pop(@makefile);
push(@makefile,"\n\n");


push(@makefile,"\nLocalLibDeps := ");
for my $guid (keys %H_projName) {

	if (exists $HoL_deps{$H_projName{$guid}}) {
		foreach my $dep(@{$HoL_deps{$H_projName{$guid}}}) {
			push(@makefile,'$(LinkDir)/lib'."$H_projName{$dep}\.a\n");
			push(@makefile,"LocalLibDeps += ");
		}
	}
}
pop(@makefile);
push(@makefile,"\n\n");
push(@makefile,"PlatformName := Ps3\n");

foreach my $config(@L_configs) {
	push(@makefile,'ifeq ($(Config),'.$config."\)\n"); 
	push(@makefile,"\t".'ConfDef := 1'."\n"); 
	push(@makefile,"endif\n");
}
push(@makefile,"\nDefault: testconfig CreateOutputDirs ");
for my $guid (keys %H_projName) {
	if (exists $HoL_deps{$H_projName{$guid}}) {
		foreach my $dep(@{$HoL_deps{$H_projName{$guid}}}) {
			push(@makefile,"$H_projName{$guid} ");
			last;
		}
	}
	push(@makefile,"$H_projName{$guid}ExtraTargets ");
}
push(@makefile,"\nbuild: Default\n");
push(@makefile,"\nrebuild: | clean Default\n");
push(@makefile,"\nrun: Default\n");
push(@makefile,"\t".'ps3run -r1000 $(LinkDir)/Exe_Main_PS3_$(Config)'."\.self\n");
push(@makefile,"\ndebug: Default\n");
push(@makefile,"\t".'ps3debugger -e -r -x $(LinkDir)/Exe_Main_PS3_$(Config)'."\.self\n");
push(@makefile,"\n".'CreateLinkDir:'."\n");
push(@makefile,"\t".'@mkdir -p $(LinkDir)'."\n\n");

push(@makefile,"contentcompile:\n\t");
push(@makefile,'s:/Projects/P5/Build/System/Win32_x86_Release/XWC_Static.exe "s:/Projects/P5/Tools/ContentCompile/ContentCompile_PS3_Full.xrg"'."\n\n");

push(@makefile,"\n".'CCacheStats:'."\n");
push(@makefile,"\t\@$ccacheDir && ccache -s\n\n");

push(@makefile,"\n".'ClearCCache:'."\n");
push(@makefile,"\t\@$ccacheDir && ccache -C -z\n\n");


push(@makefile,"testconfig:\n");
push(@makefile,'ifneq ($(ConfDef),1)'."\n\t".'@echo Please set the command line parameter Config to a valid configuration.');
push(@makefile,"\n\t".'@echo Example: $(MAKE) Config=RTM');
push(@makefile,"\n\t".'@echo Valid configs are: ');
foreach my $config(@L_configs) {
	push(@makefile,$config);
	push(@makefile,", "); 
}
pop(@makefile);
push(@makefile,"\n\t\@exit 1\n");
push(@makefile,"endif\n\n");


for my $guid (keys %H_projName) {
	my ($base,$path,$type) = fileparse($H_projDir{$guid},"\.vcproj");
	push(@makefile,"\ninclude "."makefile\.$H_projName{$guid}");
}
push(@makefile,"\n\n");



for my $guid (keys %H_projName) {
	my ($base,$path,$type) = fileparse($H_projDir{$guid},"\.vcproj");
	push(@makefile,"makefile\.$H_projName{$guid}: $H_projDir{$guid} $convSettingsFilename $relPath".'/GenerateLibMakeFile.pl'."\n");
	push(@makefile,"\t".'@echo Creating makefile for '.$H_projName{$guid}."\n");
	push(@makefile,"\t".'@perl '.$relPath.'/GenerateLibMakeFile.pl'." $H_projDir{$guid} makefile\.$H_projName{$guid} $convSettingsFilename> log\n");
	push(@makefile,"$H_projName{$guid}: makefile\.$H_projName{$guid} testconfig ".'$(CreateOutputDirs) $('.$H_projName{$guid}.'TargetPath) ');
	push(@makefile,"$H_projName{$guid}ExtraTargets ");
#	push(@makefile,"\n\t".'@echo Building: '.$H_projName{$guid});
#	push(@makefile,"\n\t\@".'$(MAKE) -f '.$path.'makefile.'.$H_projName{$guid});
	push(@makefile,"\n\n");
}

push(@makefile,"\nProjectMakefiles: ");
for my $guid (keys %H_projName) {
#	print $H_projName{$guid}."\n";
	my ($base,$path,$type) = fileparse($H_projDir{$guid},"\.vcproj");
	push(@makefile,$path."makefile\.$H_projName{$guid} ");
}
push(@makefile,"\n");

push(@makefile,"\nclean: testconfig ");
for my $guid (keys %H_projName) {
	my ($base,$path,$type) = fileparse($H_projDir{$guid},"\.vcproj");
	push(@makefile,"$H_projName{$guid}clean ");
}
push(@makefile,"\n\t".'@rm $(LinkDir)/* -f');
push(@makefile,"\n");

push(@makefile,"\nCreateOutputDirs: testconfig ".'CreateLinkDir CreateContentCompileExeDir ');
for my $guid (keys %H_projName) {
	my ($base,$path,$type) = fileparse($H_projDir{$guid},"\.vcproj");
	push(@makefile,$H_projName{$guid}."_CreateOutputDir ");
}
push(@makefile,"\n\n");

push(@makefile,"\n".'CreateContentCompileExeDir:'."\n");
push(@makefile,"\t".'@mkdir -p cd S:/Projects/P5/ContentCompile/PS3/System/PS3Exes'."\n");


@makefile = () unless @makefile;
open F, "> $makeFilename" or die "Can't open makefile for save: $makeFilename";
print F @makefile;
close F;


sub parseSettings {
	my $file = shift; 
	
	open (F, $file) || die "Can't open settingsfile: ".$file."\n";
	@File = <F>;
	close (F);

	my $parsingSpuBuild=0;
	my $parsingStub=0;
	my $parsingTranslation=0;
	my $parsingSkip=0;
	$useccache=1;
	$usedistcc=1;

	for (@File) {
		next if ($_ =~ /^#/);
		$_=~ s/\n//g;
		my @line=split(/#/,$_);
		if ($line[0]=~ /^(Translate\:)/) {
			$parsingTranslation=1;
			$parsingSkip=0;
			$parsingStub=0;
			$parsingSpuBuild=0;
		}
		elsif ($line[0]=~ /^(Skip\:)/) {
			$parsingSkip=1;
			$parsingTranslation=0;
			$parsingStub=0;
			$parsingSpuBuild=0;
		}
		elsif ($line[0]=~ /^(Stub\:)/) {
			$parsingStub=1;
			$parsingTranslation=0;
			$parsingSkip=0;
			$parsingSpuBuild=0;
		}
		elsif ($line[0]=~ /^(SpuBuild\:)/) {
			$parsingStub=0;
			$parsingTranslation=0;
			$parsingSkip=0;
			$parsingSpuBuild=1;
		}
		else
		{
			if ($parsingStub)
			{
				$line[0] =~ s/^\t//g;
				$line[0] =~ s/ +/ /g;
				$line[0] =~ s/\s*$//g;
				$useccache=0 if ($line[0] =~ /CCACHE\s*:=\s*$/);
				$usedistcc=0 if ($line[0] =~ /DISTCC\s*:=\s*$/);
				push(@makefile,$line[0]."\n");
			}
		}
	}
}


