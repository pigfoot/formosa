#!/usr/local/bin/perl

$ch = "a";
@az = ();
$parent = "..";

$oricwd = `pwd`;
$homedir = "/apps/bbs/home";

for ($i=0; $i<26; $i++) {
	push(@az, $ch);
    $ch++;
}

chdir($homedir);

foreach $name ( @az ) {
	$some_dir = $homedir."/$name";
##	print "$some_dir\n";

	@namelist = ();	

	opendir(DIR, $some_dir) || die "can't opendir $some_dir: $!";
	while ( $i = readdir(DIR) ) {
		if ($i =~ /^\./ ) {
			next;
		}
		$tmp = lc($i);
		print "$tmp\t";
		push(@namelist, $tmp);;
	}
	if ($#namelist lt 0) {
		next;
	}
#	print "$#namelist\n";	
	@newnamelist = sort @namelist;

	$outfile = "/tmp/chkdupid.tmp";
	open(OUTFILE, ">$outfile") || die "can't create $outfile: $!";
	foreach $j ( @newnamelist ) {
		print OUTFILE "$j\n";
	}
	close(OUTFILE);
	
#    @dots = grep { /^\./ && -f "$some_dir/$_" } readdir(DIR);
    closedir DIR;

##	system "sort $outfile | uniq -c | grep -v \"   1\" | sort +0nr > $outfile.$name";
	system "cp $outfile $homedir/$name/.index";

#	print @dots;
}

chdir($oricwd);

