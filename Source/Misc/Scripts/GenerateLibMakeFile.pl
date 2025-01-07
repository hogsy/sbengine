#!/bin/perl
# todo
# succes räknare, failure

use Xml::Parser;
use File::Basename;


$file=$ARGV[0];
$makefileName=$ARGV[1];
$settingsFile=$ARGV[2];

if ($#ARGV + 1 != 3) {
	print "Usage: vcprojfile destmakefile settingsfile\n";
	exit(1);
}

if (!($file=~/\.vcproj$/)) {
	print "Projectfile must have extension \".vcproj\".\n";
	exit(1);
}

if (!-e $file) {
	print "Projectfile: \"$file\" doesn't exist.\n";
	exit(1);
}

my ($base,$path,$type) = fileparse($file,"\.vcproj");
$projectDir=$path;
$projectDir=~s/\/$//;



$configCount=0;
%H_configs=();
%H_configOptions=();
%H_configIncludes=();
%H_configDefines=();
@L_globalOptions=();
@L_globalIncludes=();
@L_globalDefines=();

@L_normalFiles=();
%HoL_normalFiles=();
%H_configExcludedFile=();
%H_configFileOptions=();
%HoH_fileOptions=();
%H_fileOptions=();
%H_spuFiles=();
%H_spursFiles=();

$parsingConfig=0;
$parsingFiles=0;
$parsingFile=0;
$currentConfig=0;
$currentFile=0;
$createLib=0;
@makefile=();

$ws=""; 	#whitespace


parseSettings($settingsFile);

#$p = new XML::Parser(Style => 'Debug');
$p = new XML::Parser(ErrorContext => 2);

$p->setHandlers(Start => \&start_handler,
                End   => \&end_handler,
                Char  => \&char_handler);
$p->parsefile($file);

@L_globalOptions=findCommon(" ",\%H_configOptions);
@L_globalIncludes=findCommon(";",\%H_configIncludes);
@L_globalDefines=findCommon(";",\%H_configDefines);

removeDuplicates(" ",\%H_configOptions);
removeDuplicates(";",\%H_configIncludes);
removeDuplicates(";",\%H_configDefines);


# Collect all files

foreach my $file(@L_normalFiles) {
	$file=~s/\\/\//g;
	$SrcFilesCpp{$file}=1 if ($file =~ /.cpp$/i);
	$SrcFilesC{$file}=1 if ($file =~ /.c$/i);
	my $objName='$('.$projectName.'OutputDir)/'.fileparse($file,qr{\.\w{1,3}}).".o";
	$ObjFilesCpp{$objName}=1 if ($file =~ /.cpp$/i);
	$ObjFilesC{$objName}=1 if ($file =~ /.c$/i);
}

for my $conf (keys %HoL_normalFiles) {
	foreach my $file(@{$HoL_normalFiles{$conf}}) {
		$file=~s/\\/\//g;
		my $objName='$('.$projectName.'OutputDir)/'.fileparse($file,qr{\.\w{1,3}}).".o";
#		$ObjFilesCpp{$objName}=1 if ($file =~ /.cpp$/i);
#		$ObjFilesC{$objName}=1 if ($file =~ /.c$/i);
	}	
}

for my $file (keys %H_spuFiles) {
	$file=~s/\\/\//g;
	my $elfName='$('.$projectName.'OutputDir)/'.fileparse($file,qr{\.\w{1,3}}).".elf";
	$SpuElfFiles{$elfName}=1;
}

for my $file (keys %H_spursFiles) {
	$file=~s/\\/\//g;
	my $objName='$('.$projectName.'OutputDir)/'.fileparse($file,qr{\.\w{1,3}}).".spurs.o";
	$SpursObjFiles{$objName}=1;
}


#for my $conf (keys %HoH_fileOptions) {
#	for my $file ( keys %{$HoH_fileOptions{ $conf }} ) {
#		$allFiles{$file}=1;
#	}
#}

push(@makefile,"\n");
push(@makefile,$projectName."ProjectName := ".$projectName."\n");
push(@makefile,$projectName.'OutputDir :='.$projectDir.'/Output/$(PlatformName)/'.$projectName.'/$(Config)'."\n");

push(@makefile,"\n");
push(@makefile,$projectName."Includes := ");
foreach my $include(@L_globalIncludes) {
	$include=~s/SCE_PS3_ROOT/FIXED_SCE_PS3_ROOT/g;
	$include=~s/SN_PS3_PATH/FIXED_SN_PS3_PATH/g;
	$include=~s/\\/\//g;
	$include=~s/\"//g;
	if ($include=~/\$\(ProjectDir\)/) {
		$include=~s/\$\(ProjectDir\)//;
		$include=pathMerge($projectDir,$include);
	}
	if ($include=~/FIXED_/)	{
		push(@makefile,"-isystem ".$include."\n");
	}
	else {
		push(@makefile,"-I ".$include."\n");
	}
	push(@makefile,$projectName."Includes += ");
}
push(@makefile,"-I $projectDir\n");
#push(@makefile,"-I .\n");

push(@makefile,$projectName."PPU_CPP_OPTIONS := ");
foreach my $option(@L_globalOptions) {
	push(@makefile,$option." ");
}
push(@makefile,'$(PPU_CPP_OPTIONS)'."\n");

push(@makefile,$projectName."Defs := ");
foreach my $def(@L_globalDefines) {
	push(@makefile,"-D".$def." ");
}
push(@makefile,"\n\n");

#Objectfiles
push(@makefile,$projectName."ObjFilesCpp := ");
for my $file (sort keys %ObjFilesCpp) {
	push(@makefile,$file." ");
}
push(@makefile,"\n");
push(@makefile,$projectName."ObjFilesC := ");
for my $file (sort keys %ObjFilesC) {
	push(@makefile,$file." ");
}
push(@makefile,"\n");
push(@makefile,"\n");

push(@makefile,$projectName."SpuElfFiles := ");
for my $file (sort keys %SpuElfFiles) {
	push(@makefile,$file." ");
}
push(@makefile,"\n");
push(@makefile,"\n");

push(@makefile,$projectName."SpursObjFiles := ");
for my $file (sort keys %SpursObjFiles) {
	push(@makefile,$file." ");
}
push(@makefile,"\n");
push(@makefile,"\n");


push(@makefile,$projectName."SrcFilesCpp := ");
for my $file (sort keys %SrcFilesCpp) {
	print $file."\n";
	print $projectDir."\n";
	$file=pathMerge($projectDir,$file);
	print $file."\n";
	push(@makefile,$file." ");
}
push(@makefile,"\n");
push(@makefile,$projectName."SrcFilesC := ");
for my $file (sort keys %SrcFilesC) {
	$file=pathMerge($projectDir,$file);
	push(@makefile,$file." ");
}
push(@makefile,"\n");
push(@makefile,"\n");

for my $conf (keys %H_configs) {
	$fixedConf=$conf;
	$fixedConf=~s/ /\_/g;
	push(@makefile,"\nifeq \(\$\(Config\)\,".$fixedConf."\)\n");
	
	if (exists $HoL_normalFiles{$conf}) {
		my $first=0;
		foreach my $file(@{$HoL_normalFiles{$conf}}) {
			if ($file =~ /.cpp$/i) {
				push(@makefile,"\t".$projectName."SrcFilesCpp += ") if (!$first);
				$file=pathMerge($projectDir,$file);
				push(@makefile,$file." ");
				$first=1;
			}
		}	
		push(@makefile,"\n") if ($first);
		my $first=0;
		foreach my $file(@{$HoL_normalFiles{$conf}}) {
			if ($file =~ /.c$/i) {
				push(@makefile,"\t".$projectName."SrcFilesC += ") if (!$first);
				$file=pathMerge($projectDir,$file);
				push(@makefile,$file." ") ;
				$first=1;
			}
		}	
		push(@makefile,"\n") if ($first);
		
		my $first=0;
		foreach my $file(@{$HoL_normalFiles{$conf}}) {
			if ($file =~ /.cpp$/i) {
				push(@makefile,"\t".$projectName."ObjFilesCpp += ") if (!$first);
				my $objName='$('.$projectName.'OutputDir)/'.fileparse($file,qr{\..+}).".o";
				push(@makefile,$objName);
				$first=1;
			}
		}	
		push(@makefile,"\n") if ($first);
		my $first=0;
		foreach my $file(@{$HoL_normalFiles{$conf}}) {
			if ($file =~ /.c$/i) {
				push(@makefile,"\t".$projectName."ObjFilesC += ") if (!$first);
				my $objName='$('.$projectName.'OutputDir)/'.fileparse($file,qr{\..+}).".o";
				push(@makefile,$objName);
				$first=1;
			}
		}	
		push(@makefile,"\n") if ($first);
	}
	
	if (exists $H_configDefines{$conf} && $H_configDefines{$conf}) {
		push(@makefile,"\t".$projectName."Defs += ");
		my @defs=split(/;/,$H_configDefines{$conf}." ");
		foreach my $def (@defs) {
			push(@makefile,"-D".$def." ");
		}
		push(@makefile,"\n");
	}	
	if (exists $H_configIncludes{$conf} && $H_configIncludes{$conf}) {
		my @incs=split(/;/,$H_configIncludes{$conf}." ");
		foreach my $inc (@incs) {
			push(@makefile,"\t".$projectName."Includes += ");
			#$inc=~s/\$\(ProjectDir\)//;
			$inc=~s/\$\(ProjectDir\)//;
			$inc=pathMerge($projectDir,$inc);
			$inc=~s/\\/\//g;
			$inc=~s/\"//g;
			$inc=~s/^\///;
			push(@makefile,"-I".$inc."\n");
		}
	}	
	if (exists $H_configOptions{$conf} && $H_configOptions{$conf}) {
		push(@makefile,"\t".$projectName."PPU_CPP_OPTIONS := ");
		my @cflags=split(/;/,$H_configOptions{$conf}." ");
		foreach my $cflag (@cflags) {
			$cflag=~s/\s+/ /g;
			push(@makefile,$cflag." ");
		}
		push(@makefile,'$('.$projectName.'PPU_CPP_OPTIONS) ');
	}	
	if (exists $H_configLinkDirs{$conf} && $H_configLinkDirs{$conf}) {
		my $linkdir=$H_configLinkDirs{$conf};
		$linkdir=~s/SCE_PS3_ROOT/FIXED_SCE_PS3_ROOT/g;
		$linkdir=~s/SN_PS3_PATH/FIXED_SN_PS3_PATH/g;
		$linkdir=~s/\"//g;

		push(@makefile,"\n\t".$projectName."PPU_LD_DIRS = $linkdir");
	}	
	if (exists $H_configLinkDeps{$conf} && $H_configLinkDeps{$conf}) {
		push(@makefile,"\n\t".$projectName."PPU_LD_DEPS += $H_configLinkDeps{$conf}");
	}	
	push(@makefile,"\nendif\n");
}

push(@makefile,"\n".$projectName."clean: \n");
push(@makefile,"\t"."\@echo Cleaning $projectName".' $(Config)'."\n");
push(@makefile,"\t".'@-rm -f $('.$projectName.'OutputDir)/*.*'."\n");

# INCLUDE DEPS
push(@makefile,'-include $('.$projectName.'ObjFilesCpp:.o=.d) '.
		'$('.$projectName.'ObjFilesC:.o=.d) '.
		'$('.$projectName.'SpuElfFiles:.elf=.d)'.
		'$('.$projectName.'SpursObjFiles:.o=.d)'.
		"\n");
# SPU COMPILE
for my $file (keys %H_spuFiles) {
	$file=~s/\\/\//g;
	my $elfName=fileparse($file,qr{\.\w{1,3}}).".elf";
	push(@makefile,'$('.$projectName.'OutputDir)/'.$elfName.": ".pathMerge($projectDir,$file)."\n");
	push(@makefile,$spuBuildCommand);
}

# SPURS COMPILE
for my $file (keys %H_spursFiles) {
	$file=~s/\\/\//g;
	my $objName=fileparse($file,qr{\.\w{1,3}}).".spurs.o";
	push(@makefile,'$('.$projectName.'OutputDir)/'.$objName.": ".pathMerge($projectDir,$file)."\n");
	push(@makefile,$spursBuildCommand);
}

@makefile=(@makefile,@makefileP1);

# Create Libs
push(@makefile,"\n".$projectName.'_CreateOutputDir:'."\n");
push(@makefile,"\t".'@mkdir -p $('.$projectName.'OutputDir)'."\n");
if ($createLib) {
	push(@makefile,"\n".$projectName."TargetFileName := lib".$projectName.'.a');
	# Lib targets
	push(@makefile,"\n".$projectName."TargetPath := ".'$(LinkDir)/$('.$projectName.'TargetFileName)');
#	push(@makefile,"\n".$projectName.': $('.$projectName.'TargetPath)');
	push(@makefile,"\n");
	push(@makefile,"\n".'$('.$projectName.'TargetPath): '.
						'$('.$projectName.'ObjFilesCpp) '.
						'$('.$projectName.'ObjFilesC) '.
						'$('.$projectName.'SpursObjFiles) ');

	push(@makefile,"\n\t".'@echo Creating '.$projectName);
	# AR command line
	push(@makefile,"\n\t".'$(PPU_LN_VERBOSE)$(PPU_AR) rc $('.$projectName.'TargetPath) '.
														'$('.$projectName.'ObjFilesCpp) '.
														'$('.$projectName.'ObjFilesC) '.
														'$('.$projectName.'SpursObjFiles) ');
	# Ranlib
	push(@makefile,"\n\t".'$(PPU_LN_VERBOSE)$(PPU_RANLIB) $('.$projectName.'TargetPath)');
	push(@makefile,"\n");
		
	# Create extra targets for spu elf
	push(@makefile,"\n".$projectName.'ExtraTargets: ');
	for my $file (keys %H_spuFiles) {
		$file=~s/\\/\//g;
		my $elfName=fileparse($file,qr{\.\w{1,3}}).".elf ";
		push(@makefile,' $('.$projectName.'OutputDir)/'.$elfName);
	}
	push(@makefile,"\n");
}
# Link elf
elsif ($createExe) {
	push(@makefile,"\n".$projectName."TargetFile := ".$projectName.'_$(Config).elf');
	push(@makefile,"\n".$projectName."TargetSelfFile := ".$projectName.'_$(Config).self');
	push(@makefile,"\n".$projectName."TargetPath := ".'$(LinkDir)/$('.$projectName.'TargetFile)');
	push(@makefile,"\n".$projectName."TargetSelfPath := ".'$(LinkDir)/$('.$projectName.'TargetSelfFile)');
#	push(@makefile,"\n".$projectName.': $('.$projectName.'TargetPath)');
	# Link deps
	push(@makefile,"\n\n".'$('.$projectName.'TargetPath): $('.$projectName.'ObjFilesCpp) '.
														 '$('.$projectName.'ObjFilesC) '.
														 '$('.$projectName.'SpursObjFiles) '.
														 '$(LocalLibDeps) ');
	push(@makefile,"\n\t".'@echo Creating '.$projectName);
	# Link command line
	push(@makefile,"\n\t".'$(PPU_LN_VERBOSE)$(PPU_CC) $('.$projectName.'PPU_LD_DIRS) '.
													'-L$(LinkDir) -Wl,--start-group '.
													'$('.$projectName.'ObjFilesCpp) '.
													'$('.$projectName.'ObjFilesC) '.
													'$(LocalLibs) '.
													'$('.$projectName.'PPU_LD_DEPS) '.
													'-Wl,--end-group '.
													'-o $('.$projectName.'TargetPath)'.
													' $(LINK_ERRORCONV)');
	# Convert to self
	push(@makefile,"\n\t".'$(MAKE_SELF) $('.$projectName.'TargetPath) $('.$projectName.'TargetSelfPath)');
	# Postcopy
	push(@makefile,"\n\t".'PostCopy $('.$projectName.'TargetSelfPath) $(SolutionName) System/PS3Exes/$('.$projectName.'TargetSelfFile)');
	push(@makefile,"\n\t".'PostCopy $('.$projectName.'TargetPath) $(SolutionName) System/PS3Exes/$('.$projectName.'TargetFile)');
		#PostCopy "$(TargetPath)" $(SolutionName) "System\PS3Exes\$(TargetFileName)"
	push(@makefile,"\n");	
	push(@makefile,"\n".$projectName.'ExtraTargets: ');
	push(@makefile,"\n");	
}

else {
	push(@makefile,"\n".$projectName.":");
#	push(@makefile,"\n\t".'@echo Dummytarget: '.$projectName);
	push(@makefile,"\n");	
	push(@makefile,"\n".$projectName.'ExtraTargets: ');
	push(@makefile,"\n");	

}

writeData($makefileName,@makefile);

#print "\nGlobal options:\n";
foreach $option(@L_globalOptions) {
#	print $option."\n";
}
#print "\nConfig options:\n";
for my $conf (keys %H_configOptions) {
	my $value = $H_configOptions{$conf};
#	print $conf.":\t\t".$value."\n";
}

#print "\nGlobal options:\n";
foreach $option(@L_globalIncludes) {
#	print $option."\n";
}
#print "\nConfig options:\n";
for my $conf (keys %H_configIncludes) {
	my $value = $H_configIncludes{$conf};
#	print $conf.":\t\t".$value."\n";
}

#print "\nGlobal options:\n";
foreach $option(@L_globalDefines) {
#	print $option."\n";
}
#print "\nConfig options:\n";
for my $conf (keys %H_configDefines) {
	my $value = $H_configDefines{$conf};
#	print $conf.":\t\t".$value."\n";
}



#print "Files:\n";
foreach my $file(@L_normalFiles)
{
#	print $file."\n";
}

for my $conf (keys %H_configs) {
#	print "\n".$conf."\n";
	foreach my $file(@{$HoL_normalFiles{$conf}}) {
#		print $file."\n";
	}
}

for my $file (keys %H_fileOptions)
{
	my $value = $H_fileOptions{$file};
	print $file." = ".$value."\n";
}


for my $conf (keys %HoH_fileOptions) {
	print "\n".$conf."\n";
	for my $file ( keys %{$HoH_fileOptions{ $conf }} ) {
		my $value = $HoH_fileOptions{$conf}{$file};
		print $file." = ".$value."\n";
	}
}


sub findCommon {
	my ($separator,$H_c)=@_;
	my @L_Global=();
	for my $conf (keys %$H_c) {
		my $value = $H_c->{$conf};
		my @options=split(/$separator/,$value);
		foreach $option(@options) {
			$originalOption=$option;
			my $optCount=0;
			$option=~s/\\/\\\\/g;
			$option=~s/\"/\\\"/g;
			$option=~s/\$/\\\$/g;
			$option=~s/\(/\\\(/g;
			$option=~s/\)/\\\)/g;
#			print $option."\n";
			for my $conf2 (keys %$H_c) {
				$optCount++ if ($H_c->{$conf2}=~/$option/);
			}
			if ($optCount==$configCount)
			{
				push(@L_Global,$originalOption);
				for my $conf2 (keys %$H_c) {
					$H_c->{$conf2}=~s/$option//g;
					$H_c->{$conf2}=~s/$separator$separator/$separator/g;
					$H_c->{$conf2}=~s/^$separator//g;
					$H_c->{$conf2}=~s/$separator$//g;
				}
				
			}
		}
		last;
	}
	return @L_Global;
}	

sub removeDuplicates {
	my ($separator,$H_c)=@_;
	for my $conf (keys %$H_c) {
		my $value = $H_c->{$conf};
		my @options=split(/$separator/,$value);
		
		
		my %temp;
		foreach (@options) {
			$temp{$_} = 0;
		}
        @options = sort keys %temp;
		$H_c->{$conf}="";
		foreach (@options) {
			$H_c->{$conf}=$H_c->{$conf}."$separator".$_;
		}
		$H_c->{$conf}=~s/^$separator//g;
	}
}	


sub start_handler
{
	my $expat = shift; 
	my $element = shift;
	push(@stack,$element);
#	print $ws.$element."\n";
	$ws="\t".$ws;
	
#	print "\nStarthandler\n".$_;
	if ($element eq "VisualStudioProject")
	{
		while(@_)
		{
			$attr=shift;
			$val=shift;
			if ($attr eq "Name") { $projectName=$val;	}
		}
	}
	elsif ($element eq "Configuration")
	{
		$parsingConfig=1;
		parseConfig(@_);
	} 
	elsif ($element eq "FileConfiguration")
	{
		$parsingConfig=1;
		parseConfig(@_);
	} 
	elsif ($element eq "Files")
	{
		$parsingFiles=1;
	}
	elsif ($element eq "File")
	{
		$parsingFile=1;
		parseFile(@_);
	}
	elsif ($element eq "Tool")
	{
		if ($parsingConfig) 
		{
			parseConfigTool(@_);
		}
		if ($parsingFile) 
		{
			parseFileTool(@_);
		}
	}
	else
	{
		while  (@_) {
			my $attr = shift;
			my $val = shift;
#			print $parsingFiles." xxx ".$currentConfig.$ws.$attr." = ".$val."\n";
		}
	}
 }
 
 sub char_handler
 {
 }
 
 sub end_handler
 {
	$ws=~s/\t//;
	$popped=pop @stack;
	if ($popped eq "Configuration")
	{
		
		$parsingConfig=0;
		$currentConfig="";
	}
	elsif ($popped eq "Files")
	{
		$parsingFiles=0;
		$currentFile="";
	}
	elsif ($popped eq "File")
	{
		$parsingFile=0;
		if ($currentFile)
		{
			my $objName=fileparse($file,qr{\.\w{1,3}}).".o";

			my $excludeCount=0;
			my $aconf=0;
			for my $conf (keys %H_configs) {
				$excludeCount++ if ( exists $H_configExcludedFile{$conf} && $H_configExcludedFile{$conf});
				$aconf=$conf;
			}
			push(@L_normalFiles,$currentFile) if ($excludeCount==0);
			if ($excludeCount>0 && $excludeCount<$configCount) {
				for my $conf (keys %H_configs) {
#					print "1 $conf $excludeCount  $currentFile \n" if ( exists $H_configExcludedFile{$conf} && $H_configExcludedFile{$conf}==0); 
#					print "2 $conf $excludeCount  $currentFile \n" if ( !exists $H_configExcludedFile{$conf}); 
					push(@{$HoL_normalFiles{$conf}},$currentFile) if ( exists $H_configExcludedFile{$conf} && $H_configExcludedFile{$conf}==0);
					push(@{$HoL_normalFiles{$conf}},$currentFile) if ( !exists $H_configExcludedFile{$conf});
				}
			}
			
			my $fileConfCount=0;
			my $optionX=$H_configFileOptions{$aconf};
			for my $conf (keys %H_configs) {
				$fileConfCount++ if ( exists $H_configFileOptions{$conf} && $H_configFileOptions{$conf} && $H_configFileOptions{$conf} eq $optionX);
#				print "conf:".$conf ." opt: ".$H_configFileOptions{$conf}."\n";
			}
			$H_fileOptions{$currentFile}=$optionX if ($fileConfCount==$configCount);
			#print $fileConfCount."  ".$currentFile."\n";
			if ($fileConfCount>0 && $fileConfCount<$configCount) {
				for my $conf (keys %H_configs) {
					#print "conf:".$conf ." opt: ".$H_configFileOptions{$conf}."\n";
					$HoH_fileOptions{$conf}{$currentFile}=$H_configFileOptions{$conf} if ( exists $H_configFileOptions{$conf} && $H_configFileOptions{$conf});
					#print $HoH_fileOptions{$conf}{$currentFile}."\n";
				}
			}
			$currentFile=~s/\\/\//g;
			my $objName=fileparse($currentFile,qr{\.\w{1,3}}).".o";
			for my $conf (keys %H_configs) {
				my $excluded=1;
				$excluded=0 if ( exists $H_configExcludedFile{$conf} && $H_configExcludedFile{$conf}==0);
				$excluded=0  if ( !exists $H_configExcludedFile{$conf});
				if (!$excluded) {
					$options='$('.$projectName.'PPU_C_OPTIONS) ' if ($currentFile=~/.c$/i);
					$options='$('.$projectName.'PPU_CPP_OPTIONS) ' if ($currentFile=~/.cpp$/i);
					$options=$options.$H_configFileOptions{$conf}." " if ($H_configFileOptions{$conf});
					$conf=~s/ /_/;
					push(@makefileP1,"\n".pathMerge($projectDir.'/Output/Ps3/'.$projectName.'/'.$conf,$objName).": ".pathMerge($projectDir,$currentFile)."\n");
					push(@makefileP1,"\t".'@echo PPU-CC $<'."\n");
					push(@makefileP1,"\t".'$(PPU_CC_VERBOSE)$(BuildPrefix) $(PPU_CC) $('.$projectName.'Defs) $('.$projectName.'Includes) '.$options.'$(COMPILE) -o $@ $< $(CC_ERRORCONV)'."\n");
					push(@makefileP1,"\t".'@$(PPU_CC) $('.$projectName.'Defs) $('.$projectName.'Includes) $(DEPEND) $@ $< > $(basename $@).d'."\n");
				}
			}
			
#			print "file: ".$currentFile."\n";
#			print "tot: ".$configCount."\n";
#			print "exc: ".$excludeCount."\n";
#			print "fcc: ".$fileConfCount."\n";
			$currentFile="";
		}
		for my $conf (keys %H_configs) {
			$H_configExcludedFile{$conf}=0;
			$H_configFileOptions{$conf}="";
		}
	}
 }
  

sub parseConfig
{
	while (@_)
	{
		my $attr=shift;
		my $val=shift;
		if ($attr eq "Name") { 
			$currentConfig=$val;
			if ($currentConfig=~/PS3/ && 
				!($currentConfig=~/Xenon/) &&
				!($currentConfig=~/Xbox 360/)) {
				$currentConfig=~s/(PS3 )//;
				$currentConfig=~s/(\|Win32)//;
				if (!exists $H_configs{$currentConfig})
				{
					$H_configs{$currentConfig}=$configCount;
					$configCount++;
				}
			}
			else {
				$currentConfig="";
			}
		}
		next if !($currentConfig);
		if ($parsingFile) {
			next if ($H_configExcludedFile{$currentConfig});
			if (lc($attr) eq lc("ExcludedFromBuild") && lc($val) eq lc("True")) {
				$H_configExcludedFile{$currentConfig}=1;
				next;
			}
#			print "fileconfig: ".$currentConfig."\t\t".$attr." = ".$val."\n";
		}
		else
		{
#			print "config: ".$currentConfig."\t".$ws.$attr." = ".$val."\n";
		}
	}
}

sub parseFile
{
	while (@_)
	{
		my $attr=shift;
		my $val=shift;
		if ($attr eq "RelativePath") 
		{
			if ($val=~/(\.cpp$)/i || $val=~/(\.c$)/i || $val=~/(\.s$)/i) {
				$currentFile=$val;	
			}
			elsif ($val=~/(\.h$)/i || $val=~/(\.inl$)/i) {
			}
			else {
				print "Warning file: ".$val." has unknown extension\n";
			}
		}
	}
}

sub parseConfigTool
{
	while(@_)
	{
		$attr=shift;
		$val=shift;
		next if !($currentConfig);
		if ($attr eq "Name") { $currentTool=$val;	}
		if (!$parsingFile) {
			if ($currentTool eq "VCCLCompilerTool") {
				my $vall=$H_configs{$currentConfig};
				#configOptions[$val];
				my $option=convertOption($attr,$val);
				if ($option) {
					$H_configOptions{$currentConfig}=$H_configOptions{$currentConfig}.$option." ";
				}
			}
			elsif ($currentTool eq "VCLibrarianTool") {
				$createLib=1;
			}
			elsif ($currentTool eq "VCLinkerTool") {
				$createExe=1;
				print $currentConfig."\n";
				if ($attr eq "AdditionalDependencies") {
					$val=~s/\"s:.*ppu.*\"//i;
					$H_configLinkDeps{$currentConfig}=$val;
					print "Deps: $val\n";
				}
				if ($attr eq "AdditionalLibraryDirectories") {
					my @tmpLD=split(/;/,$val);
					my $newLD="";
					foreach my $ld(@tmpLD) {
						$newLD=$newLD.'-L"'.$ld.'" ';
					}
					$H_configLinkDirs{$currentConfig}=$newLD;
#					print "LinkDirs: $val\n";
				}
				
			}
			elsif ($currentTool eq "VCCustomBuildTool") {
#				print "Custom: ".$currentTool."\t\t".$attr." = ".$val."\n";
			}
			else {
#				print "Unknown tool: ".$currentTool."\t\t".$attr." = ".$val."\n";
			}
		}
		else {
			next if ($H_configExcludedFile{$currentConfig});
			if ($currentTool eq "VCCLCompilerTool") {
				my $option=convertOption($attr,$val);
				if ($option) {
					$H_configFileOptions{$currentConfig}=$H_configFileOptions{$currentConfig}.$option." ";
				}
#				print "File CC tool: ".$option."\t".$attr." = ".$val."\n";
			}
			elsif ($currentTool eq "VCLibrarianTool") {
				$createLib=1;
			}
			elsif ($currentTool eq "VCLinkerTool") {
				$createExe=1;
			}
			elsif ($currentTool eq "VCCustomBuildTool") {
				if ($attr eq Outputs && $val=~/SPU_ELF/) {
					$H_spuFiles{$currentFile}=1;
					$H_configExcludedFile{$currentConfig}=1;
				}
				elsif ($attr eq Outputs && $val=~/SPURS/) {
					$H_spursFiles{$currentFile}=1;
					$H_configExcludedFile{$currentConfig}=1;
				}

#				print "File Custom: ".$currentTool."\t\t".$attr." = ".$val."\n";
			}
			else {
#				print "File Unknown tool: ".$currentTool."\t\t".$attr." = ".$val."\n";
			}
		}
	}
}

sub parseFileTool
{
	while(@_)
	{
		$attr=shift;
		$val=shift;
		next if !($currentConfig);
		next if ($H_configExcludedFile{$currentConfig});
		if ($attr eq "Name") { $currentTool=$val;	}
#			next if ($currentTool ne "VCCLCompilerTool" and
#					 $currentTool ne "VCLibrarianTool");
		if ($currentTool eq "VCCustomBuildTool") {
			if ($attr eq Outputs && $val=~/.elf/) {
				$H_spuFiles{$currentFile}=1;
				$H_configExcludedFile{$currentConfig}=1;
			}
		}
#		print "filetool: ".$currentTool.$attr." = ".$val."\n";
	}
}

sub convertOption {
	my $keyword=lc(shift);
	my $value=shift;
	
	if ($keyword eq lc("AdditionalOptions"))	{
		return $value;
	}
	
	if (!$parsingFile && $keyword eq lc("AdditionalIncludeDirectories"))	{
		$H_configIncludes{ $currentConfig }=$value;
		return "";
	}
	if (!$parsingFile && $keyword eq lc("PreprocessorDefinitions"))	{
		$H_configDefines{ $currentConfig }=$value;
		return "";
	}
	if (!exists $keywordTable{$keyword}) {
		print "\nwarning unknown option: ".$keyword."\n\n";
		return "";
	}
	elsif ($keywordTable{$keyword})
	{
		$option=$mappingTable{ $keyword."=".lc($value) };
		if ($option) {
#			print $option." <- ".$keyword."=".$value."\n";
			return $option;
		}
		else {
			print "warning unknown option: ".$keyword."=".$value."\n";
			return "";
		}
	}
	else {
#		print "Skiping: ". $keyword."\n";
		return "";
	}
}

sub parseSettings {
	my $file = shift; 
	
	open (F, $file) || die "Can't open settingsfile: ".$file."\n";
	@File = <F>;
	close (F);

	my $parsingSpuBuild=0;
	my $parsingSpursBuild=0;
	my $parsingStub=0;
	my $parsingTranslation=0;
	my $parsingSkip=0;
	for (@File) {
		next if ($_ =~ /^#/);
		$_=~ s/\n//g;
		my @line=split(/#/,$_);
		if ($line[0]=~ /^(Translate\:)/) {
			$parsingTranslation=1;
			$parsingSkip=0;
			$parsingStub=0;
			$parsingSpuBuild=0;
			$parsingSpursBuild=0;
		}
		elsif ($line[0]=~ /^(Skip\:)/) {
			$parsingSkip=1;
			$parsingTranslation=0;
			$parsingStub=0;
			$parsingSpuBuild=0;
			$parsingSpursBuild=0;
		}
		elsif ($line[0]=~ /^(Stub\:)/) {
			$parsingStub=1;
			$parsingTranslation=0;
			$parsingSkip=0;
			$parsingSpuBuild=0;
			$parsingSpursBuild=0;
		}
		elsif ($line[0]=~ /^(SpuBuild\:)/) {
			$parsingStub=0;
			$parsingTranslation=0;
			$parsingSkip=0;
			$parsingSpuBuild=1;
			$parsingSpursBuild=0;
		}
		elsif ($line[0]=~ /^(SpursBuild\:)/) {
			$parsingStub=0;
			$parsingTranslation=0;
			$parsingSkip=0;
			$parsingSpuBuild=0;
			$parsingSpursBuild=1;
		}
		else
		{
			if ($parsingStub)
			{
				#$line[0] =~ s/^\t//g;
				#$line[0] =~ s/  / /g;
				#push(@makefile,$line[0]."\n");
			}
			elsif ($parsingSpuBuild) {
				$spuBuildCommand=$spuBuildCommand.$line[0]."\n";
			}
			elsif ($parsingSpursBuild) {
				$spursBuildCommand=$spursBuildCommand.$line[0]."\n";
			}
			else {
				$line[0] =~ s/^\t//g;
				$line[0] =~ s/\s//g;
				my @splitedLine=split(/::/,$line[0]);
				my @left=split(/=/,lc($splitedLine[0]));
				next if (!$left[0]);
				if ($parsingTranslation)
				{
					$keywordTable{ $left[0] } =1;
					$mappingTable{ $left[0]."=".$left[1] } = $splitedLine[1];
				}
				elsif ($parsingSkip)
				{
					$left[0] =~ s/\s//g;
					$left[1] =~ s/\s//g;
					$keywordTable{ $left[0] } =0;
				}
			}
		}
	}
	for my $keyword (keys %keywordTable)
	{
		my $value = $keywordTable{$keyword};
#		print $keyword." = ".$value."\n";
	}
	for my $mapping (keys %mappingTable)
	{
		my $value = $mappingTable{$mapping};
#		print $mapping." = ".$value."\n";
	}
#@translationTable
#@skipTable
}

sub writeData {
	my ( $f, @data ) = @_;
	@data = () unless @data;
	open F, "> $f" or die "Can't open $f : $!";
	print F @data;
	close F;
}

sub pathMerge {
	my $p0=shift;
	my $p1=shift;
	my @p0L=split(/\//,$p0);
	my @p1L=split(/\//,$p1);
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