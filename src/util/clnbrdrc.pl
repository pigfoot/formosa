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

$max = 0;
$total = 0;
$count = 0;

foreach $name ( @az ) {
	$some_dir = $name;
##	print "$some_dir\n";

	opendir(DIR, $some_dir) || die "can't opendir $some_dir: $!";
	while ( $i = readdir(DIR) ) {
		if ($i =~ /^\./ ) {
			next;
		}
		$newname = "./$some_dir/$i/readrc";
##		print "$newname\t";

##		@stlist = stat("/etc/hosts") || die "can't stat";

                 ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
                    $atime,$mtime,$ctime,$blksize,$blocks)
                        = stat($newname);
#		print "($size)";
		if ( $size > $max ) {
			$max = $size;
		}
		$total = $total + $size;
		$count = $count + 1;
	}
	closedir(DIR);
}

print "\nCount: $count \nTotal: $total\nMax: $max";
print "\nAverage: ";
print $total/$count;
print "\n";

chdir($oricwd);

