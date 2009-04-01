#!/usr/local/bin/perl

$debug = 1;

$msginfo_dir = "msginfo";
$outfile = "msginfo.c";


die "syntax:\nmkmsginfo.pl [-i] [-m]\n" if (@ARGV < 1);

open(WFILE,">$outfile") || die "cannot write file $outfile: $!\n";

print WFILE "
/*
   BBS Message Text define for multi-language version
*/";

sub create_tmp 
{
	if ($_[1] eq "")
	{
		return -1;
	}
	if (!open(TMP, ">$_[0]"))
	{
		printf "canno create file $_[0]: $!\n";
		return -1;
	}

	$str = substr($_[2], 0, rindex($_[2], ";"));
	print TMP "$str\n";

	close(TMP);
	return 0;
}


@dirs = <$msginfo_dir/*>;

foreach $dir ( @dirs )
{
	print WFILE "\n\n/* $dir */";

	@files = glob("$dir/*");
	
	foreach $file ( @files )
	{
		if (index($file, "~") == length($file)-1)
		{
			unlink($file);
			next;
		}

		$fname = substr($file, rindex($file,"/") +1);

		if (!open(RFILE, $file))
		{
			print "cannot open file $file: $!\n"; 
			next;
		}

		if ($fname eq "index")
		{
			if ( $ARGV[1] eq "-m" )
			{
				next;
			}
			$tmp_str = "";
			$fn_tmp = "";
			$multi = 0;
			while ( <RFILE> )
			{
				chomp;
			    if ( /\*(\w+)\s*=\s*(\".*)/  || /(\w+)\[\]\s*=\s*(\".*)/)
				{
					create_tmp("$dir/$fn_tmp", $fn_tmp, $tmp_str);
					$fn_tmp = $1;
					$multi = 0;
					$tmp_str = $2;
					if ($2 eq "\"\\")
					{
						$multi = 1;
						$tmp_str .= "\n";
					}
					next;
				}
				if ($multi)
				{
					$tmp_str .= "$_\n";
				}
			}	
			close(RFILE);
		
			create_tmp("$dir/$fn_tmp", $fn_tmp, $tmp_str);
			next;
		}

		if ( $ARGV[1] eq "-i" )
		{
			next;
		}

		$string = "";
		$n_line = 0;
		while ( <RFILE> )
		{
			if ($_ ne "\n")
			{
				$string = $string.$_;
				$n_line += 1;
			}
		}
		close(RFILE);

		$_ = $string;
		chomp;
		$string = $_;

		$output = "\nchar ";
		if ( $fname !~ /\[\]/ )
		{
#			$output .= "*"."$fname = $string;";
			$output .= "$fname"."[] = $string;";
		}
		else
		{
			$output .= "$fname = $string;";
		}
#print $string;
		if ($n_line > 1)
		{
			$output .= "\n";
		}
		print WFILE $output;
	}
}

print WFILE "\n\n/* end of file */\n";

close(WFILE);
