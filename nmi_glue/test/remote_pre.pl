#!/usr/bin/perl

######################################################################
# $Id: remote_pre.pl,v 1.1.2.1 2004-06-25 01:38:10 wright Exp $
# script to set up for Condor testsuite run
######################################################################

use Cwd;
use Env; 

my $BaseDir = $ENV{BASE_DIR} || die "BASE_DIR not in environment!\n";
my $SrcDir = $ENV{SRC_DIR} || die "SRC_DIR not in environment!\n";

my $logsize = "50000000"; # size for logs of personal Condor


######################################################################
# untar pre-built tarball
######################################################################

print "Untarring RELEASE tarball\n";
chdir("$BaseDir") || die "Can't chdir($BaseDir): $!\n";
my $release_tarball = $ENV{"_NMI_BUILD_condor_binary_tarball"} ||
    die "_NMI_BUILD_condor_binary_tarball not in environment!\n";
print "Release tarball is: $release_tarball\n";

my $configure;
my $reltar;
open( UNTAR, "tar -zxvf $release_tarball |" ) ||
     die "Can't untar $release_tarball: $!\n";
print "Untarring $release_tarball ...\n";
while( <UNTAR> ) {
  if( /.*\/condor_configure/ ) {
    $configure = "$BaseDir/$_";
     chomp( $configure );
  }
  if( /.*\/release.tar/ ) {
    $reltar = "$BaseDir/$_";
    chomp( $reltar );
  }
  print;
}
close UNTAR;


######################################################################
# setup the personal condor
######################################################################

# find the version - macosx doesn't work w/the "old" way  
# XXX TODO XXX !!!!
# this seems like a hack, maybe we can find a better way...
my $version = substr($release_tarball, 7, 5);
print "VERSION string is $version\n";
# XXX TODO XXX !!!!

my $configure = "$BaseDir/condor-$version/condor_configure";
my $reltar = "$BaseDir/condor-$version/release.tar";

print "SETTING UP PERSONAL CONDOR\n";
mkdir( "$BaseDir/local" ) || die "Can't mkdir $BaseDir/local: $!\n";
mkdir( "$BaseDir/condor" ) || die "Can't mkdir $BaseDir/condor: $!\n";

print "RUNNING condor_configure\n";
$conf = &verbose_system("$configure --make-personal-condor --local-dir=$BaseDir/local --install=$reltar --install-dir=$BaseDir/condor");
($conf == 0) || die "Problem installing Personal Condor. Return val $conf\n";

# make sure ports for Personal Condor are valid  
# high ports are a concern for irix, dux and aix. 
my $col_port = $PID;
if ($col_port > 65535){
   $col_port = int($col_port / 10000) + 1024;
} 
if ($col_port < 1024){
   $col_port = $col_port + 1024;
}
my $neg_port = $col_port + 1;

rename( "$BaseDir/local/condor_config.local",
	"$BaseDir/local/condor_config.local.orig" )
    || die "Can't rename condor_config.local: $!\n";

open( ORIG, "<$BaseDir/local/condor_config.local.orig" ) ||
    die "Can't open $BaseDir/local/condor_config.local.orig: $!\n";
open( FIX, ">$BaseDir/local/condor_config.local" ) ||
    die "Can't open $BaseDir/local/condor_config.local: $!\n";
while( <ORIG> ) {
  if( /CONDOR_HOST.*/ ) {
    print FIX;
    print FIX "COLLECTOR_HOST = \$(CONDOR_HOST):$col_port\n";
    print FIX "NEGOTIATOR_HOST = \$(CONDOR_HOST):$neg_port\n";
  } else {
    print FIX;
  }
}

# ADD size for log files and debug level
# default settings are in condor_config, set here to override 
print FIX "ALL_DEBUG               = ";

print FIX "MAX_COLLECTOR_LOG       = $logsize\n";
print FIX "COLLECTOR_DEBUG         = ";

print FIX "MAX_KBDD_LOG            = $logsize\n";
print FIX "KBDD_DEBUG              = ";

print FIX "MAX_NEGOTIATOR_LOG      = $logsize\n";
print FIX "NEGOTIATOR_DEBUG        = D_MATCH\n";
print FIX "MAX_NEGOTIATOR_MATCH_LOG = $logsize\n";

print FIX "MAX_SCHEDD_LOG          = $logsize\n";
print FIX "SCHEDD_DEBUG            = D_COMMAND";

print FIX "MAX_SHADOW_LOG          = $logsize\n";
print FIX "SHADOW_DEBUG            = D_FULLDEBUG\n";

print FIX "MAX_STARTD_LOG          = $logsize\n";
print FIX "STARTD_DEBUG            = D_COMMAND\n";

print FIX "MAX_STARTER_LOG         = $logsize\n";
print FIX "STARTER_DEBUG           = D_NODATE\n";

print FIX "MAX_MASTER_LOG          = $logsize\n";
print FIX "MASTER_DEBUG            = D_COMMAND\n";

close ORIG;
close FIX; 

print "PERSONAL CONDOR installed!\n";


######################################################################
# setup the src tree so the test suite finds condor_compile and its
# other dependencies.  
######################################################################

chdir( "$SrcDir/condor_syscall_lib" )
    || die "Can't chdir($SrcDir/condor_syscall_lib): $!\n";
opendir( DIR, "$BaseDir/condor/lib" ) ||
  die "can't opendir $BaseDir/condor/lib for condor_syscall_lib: $!\n";
@files = readdir(DIR);
closedir DIR;
foreach $file ( @files ) {
    if( $file =~ /.*\.[oa]/ ) {
	symlink( "$BaseDir/condor/lib/$file", "$file" ) ||
	    die "Can't symlink($BaseDir/condor/lib/$file): $!\n";
    }
}

chdir( "$SrcDir/condor_scripts" ) || 
    die "Can't chdir($SrcDir/condor_scripts): $!\n";

unlink( "condor_compile" ) || die "Can't unlink(condor_compile): $!\n";
unlink( "ld" ) || die "Can't unlink(ld): $!\n";
symlink( "$BaseDir/condor/lib/ld", "ld" ) ||
    die "Can't symlink($BaseDir/condor/lib/ld): $!\n";
symlink( "$BaseDir/condor/bin/condor_compile", "condor_compile" ) ||
    die "Can't symlink($BaseDir/condor/lib/condor_compile): $!\n";


######################################################################
# spawn the personal condor
######################################################################

chdir($BaseDir);
print "Starting condor_master, about to FORK in $BaseDir\n";

$master_pid = fork();
if( $master_pid == 0) {
  exec("$BaseDir/condor/sbin/condor_master -f");
  print "MASTER EXEC FAILED!!!!!\n";
  exit 1;
}

# save the master pid for later use
print "Master PID is $master_pid\n";
open( PIDFILE, ">$BaseDir/condor_master_pid") || 
    die "cannot open $BaseDir/condor_master_pid: $!\n";
print PIDFILE "$master_pid";
close PIDFILE;   

# Give the master time to start before jobs are submitted.
print "Sleeping for 30 seconds to give the master time to start.\n";
sleep 30;


sub verbose_system {

  my @args = @_;
  my $rc = 0xffff & system @args;

  printf "system(%s) returned %#04x: ", @args, $rc;

  if ($rc == 0) {
   print "ran with normal exit\n";
   return $rc;
  }
  elsif ($rc == 0xff00) {
   print "command failed: $!\n";
   return $rc;
  }
  elsif (($rc & 0xff) == 0) {
   $rc >>= 8;
   print "ran with non-zero exit status $rc\n";
   return $rc;
  }
  else {
   print "ran with ";
   if ($rc &   0x80) {
       $rc &= ~0x80;
       print "coredump from ";
   return $rc;
   }
   print "signal $rc\n"
  }
}

