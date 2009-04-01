#!/usr/local/bin/perl

$debug = 0;

$infile = @ARGV[0];
$tag = $infile;
$tag =~ s/\.c$//;
$outfile = ($tag.".strip");

print "infile: $infile\n";
print "outfile: $outfile\n";

open(RFILE,"$infile") || die "cannot open file $infile: $!\n";
open(WFILE,">$outfile") || die "cannot write file $outfile: $!\n";

$level = 0;
$line = 0;
$match = 0;
$match_in_level = -1;
$visible = 1;


print WFILE "/* 
 * FormosaBBS 
 * Version 1.1.0
 * Dec 1997
 * 
 * Li-te Huang
 * lthuang\@cc.nsysu.edu.tw
 * National Sun Yat-sen University
 * Republic Of China
 *
 * \$filename: $infile
 * 
 */
";

@define = qw(NSYSUBBS HAVE_TELECOMM HAVE_BUG DEBUG ACTFILE SHOW_UPTIME SYSOP_BIN DELUSER
CSBBS);

while (<RFILE>)
{
	chomp;

	$line += 1;

	if ( /#if/ )
	{
		$level += 1;

		if ($match == 0)
		{
			if ( /#if\s.*0\s*/ )
			{
				if ($debug)
				{
					printf "match \$$line: $_\n";
				}
				$match = 1;
				$match_in_level = $level;
				$visible = 0;
				next;
			}
			elsif ( /#if\s.*1\s*/ )
			{
				if ($debug)
				{
					printf "match \$$line: $_\n";
				}

				$match = 1;
				$match_in_level = $level;
				$visible = 1;
				next;
			}

			foreach $i ( @define )
			{
				if ( /#ifdef\s.*$i\s*/ )
				{
					if ($debug)
					{
						printf "match level$level/line$line: $_\n";
					}
					$match = 1;
					$match_in_level = $level;
					$visible = 0;
					next;
				}
				elsif ( /#ifndef\s.*$i\s*/ )
				{
					if ($debug)
					{
						printf "match level$level/line$line: $_\n";
					}

					$match = 1;
					$match_in_level = $level;
					$visible = 1;
					next;
				}
			}
		}
		if ($match == 0)
		{
			if ($debug)
			{
				print WFILE "visible level$level/line$line: $_\n";
			}
			else
			{
				print WFILE "$_\n";
			}
		}

	}
	elsif ( /#endif/ )
	{
		if ($match)
		{
			if ( $level == $match_in_level)
			{
				$match = 0;
				$match_in_level = -1;
				$visible = ~$visible;
				if ($debug)
				{
					printf "endif level$level/line$line: $_\n";
				}
			}
		}
		else
		{
			if ($debug)
			{
				print WFILE "endif level$level/line$line: $_\n";
			}
			else
			{
				print WFILE "$_\n";
			}
		}
		$level -= 1;
	}
	else
	{
		if ($visible)
		{
			if ($debug)
			{
				print WFILE "visible level$level/line$line: $_\n";
			}
			else
			{
				print WFILE "$_\n";
			}
		}
	}
}

close(RFILE);
close(WFILE);
