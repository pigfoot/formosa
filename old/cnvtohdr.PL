#!/usr/local/bin/perl

$debug = 0;

$infile = @ARGV[0];
$tag = $infile;
$tag =~ s/\.c$//;
$outfile = ($tag.".h");

print "infile: $infile\n";
print "outfile: $outfile\n";

open(RFILE,"$infile") || die "cannot open file $infile: $!\n";
open(WFILE,">$outfile") || die "cannot write file $outfile: $!\n";

$multi = 0;
$lastline = 1;

$_ = $tag;
tr/a-z/A-Z/;
$uppertag = $_;

print WFILE "\n#ifndef _BBS_".$uppertag."_H_\n#define _BBS_".$uppertag."_H_\n\n";

while (<RFILE>)
{
	chomp;
	if ($multi)
	{
		if (!$lastline)
		{
			$i = substr($_, rindex($_,";")); 
			if ( $i =~ /^;\s*$/ )
#			if ( /;/ )
			{
				$lastline = 1;
			}
			next;
		}
	}

	$multi = 0;
	
	if ( /(.*)\=.*\"\\\s*$/ ) 
	{
		if ($debug)
		{
			print "multi-line: $1\n";
		}
		else
		{
			print WFILE "extern $1;\n";
		}
		$multi = 1;
		$lastline = 0;
	}
	elsif ( /(.*)\=.*\".*\".*;/ ) 
	{
		if ($debug) 
		{ 
			print "quote-line: $1\n";
		}
		else
		{
			print WFILE "extern $1;\n";
		}
	}
	elsif ( /(.*)\=.*;/ ) 
	{
		if ($debug) 
		{ 
			print "single-line: $1\n";
		}
		else
		{
			print WFILE "extern $1;\n";
		}
	}
	elsif ( /;/ ) 
	{
		print WFILE "extern $_\n";
	}
	else
	{
		print WFILE "$_\n";
	}
}

print WFILE "\n#endif /* _BBS_".$uppertag."_H_ */\n";

close(RFILE);
close(WFILE);
