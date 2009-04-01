#!/usr/local/bin/perl
require("/u/staff/bbsroot/csie_util/bntpd/innd/bbslib.pl");
$inndhome = "/u/staff/bbsroot/csie_util/bntpd/innd";
&initial_bbs();
$i=0;
while (<>) {
  chop;
  $board = $_;
  ($group, $server) = &search_board( $board );
#  print "$group $server\n";
  @Servers = split(/\s+/,$server);
  foreach $server (@Servers) {
  if( $server ) {
	  ($serveraddr, $servername,$pro) = &search_nodelist( $server );
  } else {
    $path = "$mybbsid (local)";
    $serveraddr = "";
  }
  printf "%3d %-20s %s %s %s %s\n", $i++,$board,$serveraddr,$server, $servername, $pro;
  }
}
