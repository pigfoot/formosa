while (<>) {
        $init = undef;
        if (/^#define/) {
                next;
        } elsif (/^#ifndef/) {
                next;
        } elsif (/^#endif/) {
                next;
        } elsif (/^#/) {
                print;
                next;
        }

        if (/^.\*/) {
                print;
                next;
        }
#        next unless (/./);
		if (! /./ )	{
			print "\n";
			next;
		}
        next if (/\[\];$/);
        die unless (/^extern\s+([^;]+);(.*)$/);
        $var = $1;
        $comments = $2;
        if ($comments =~ m+/\*\s*(.*)\s*\*/+) {
                $init = $1;
                $init =~ s/\s$// while ($init =~ /\s$/);
        }
        print $var;
        print " = $init" if (defined $init);
        print ";\n";
}
exit 0;
