#!/usr/bin/perl -w
#---------------------------------------------------------------------------
# $Id$
#---------------------------------------------------------------------------

# uncramfs.pl

# start download of root.cramfs.gz then
# tail -fc +0 root.cramfs.gz |gunzip|../uncramfs.pl|less

# for uncompressed
# tail -fc +0 root.cramfs |../uncramfs.pl|less

my $fn = shift @ARGV;

# some definitions
my $inode_size = 4+4+4;
my $super_size = 4+4+4+4+16+16+16;

my $address=0;

#subs
sub decode{
    my ($buf, @pat) = @_;

    my @types;
    my @names;
    
    while (@pat){
	push @types, shift @pat;
	push @names, shift @pat;
    }
    my $spat = join '', @types;
    my (@values) = unpack $spat, $buf;
    
#    print "pattern ",Dumper($spat, \@values);
    
    my $result = {map {$_ => shift @values} @names};
    return $result;
}

# test for directoryness
sub is_directory{
    my ($modebits) = @_;

    return ($modebits & 0x4000);
}

sub rwxbits {
    my ($modebits) = @_;
    my $result =
	(($modebits & 0x4)?'r':'-').
	(($modebits & 0x2)?'w':'-').
	(($modebits & 0x1)?'x':'-');
    return $result;
}

sub fancymodebits {
    my ($modebits) = @_;

    my $result;
    
    if ($modebits & 0x2000){
	$result = 'l';
    }elsif($modebits & 0x4000){
	$result = 'd';
    }else{
	$result = '-';
    }

    my $or;
    if ($modebits & 0x800){
	$or->{u} = 's';
    }
    if ($modebits & 0x400){
	$or->{g} = 's';
    }

    $result .= rwxbits($modebits>>6);
    $result .= rwxbits($modebits>>3);
    $result .= rwxbits($modebits);
}
sub main {
    if ($fn){
	close STDIN;
	open STDIN, "<$fn";
    }
    
    my $cramfs_super='';
    while (length($cramfs_super) < $super_size){
	my $nread = sysread(STDIN,
			    $cramfs_super,
			    $super_size - length($cramfs_super),
			    length($cramfs_super));
	die "read failed"
	    if ($nread < 0);
	$address += $nread;
#	print "read $nread\n";
    }

    # NB use 'L' which is endian-sensitive, this matches
    # cramfs being endian sensitive.
    my $cramfs_s = decode($cramfs_super, (L=>'magic',
					  L=>'size',
					  L=>'flags',
					  L=>'future',
					  A16=>'sig',
					  A16=>'fsid',
					  A16=>'name'
					  ));
    
    # checks
    die "bad magic number" unless
	($cramfs_s->{magic} == 0x28cd3d45);
#    die "unexpected size" unless
#	($cramfs_s->{size} == 0x10000);
    die "unexpected future" unless
	($cramfs_s->{future} == 0x0);
#    die "unexpected flags" unless
#	($cramfs_s->{flags} == 0x0);

    use Data::Dumper;
#    print Dumper($cramfs_s);
    printf "CRAMFS volume ID 0x%s ", (unpack 'H*', $cramfs_s->{fsid});
    print "'",$cramfs_s->{name},"'\n";
    if ($cramfs_s->{size} != 0x10000){
	    print "size: ",$cramfs_s->{size},"\n";
    }
    if ($cramfs_s->{flags} != 0x0){
	    printf "flags: 0x%-8.8x\n",$cramfs_s->{flags};
    }

    # read inodes
    my $fileOffsets;
    my $dirOffsets;
    
    my $currentPath = '.';

    while(1){
	my $cramfs_inode='';
	while (length($cramfs_inode) < $inode_size){
	    my $nread = sysread(STDIN,
				$cramfs_inode,
				$inode_size - length($cramfs_inode),
				length($cramfs_inode));
	    die "read failed"
		if ($nread < 0);
	    $address += $nread;
#	    print "read $nread\n";
	}
	
	my $cramfs_i = decode($cramfs_inode, (L=>'modeuid',
					      L=>'sizegid',
					      L=>'namelenoffset')
			     );
	
	# unpack the C compiler's packed fields
	$cramfs_i->{mode} = ($cramfs_i->{modeuid} & 0xffff);
	$cramfs_i->{uid} = ($cramfs_i->{modeuid} & 0xffff0000)>>16;

	$cramfs_i->{size} = ($cramfs_i->{sizegid} & 0xffffff);
	$cramfs_i->{gid} = ($cramfs_i->{sizegid} & 0xff000000)>>24;

	$cramfs_i->{namelen} = ($cramfs_i->{namelenoffset} & 0x3f);
	$cramfs_i->{offset} = ($cramfs_i->{namelenoffset} & 0xffffffc0)>>6;


	if ($cramfs_i->{namelen}){

	    # read filename
	    my $namebuflen = $cramfs_i->{namelen} * 4;
	    my $filename='';
	    while (length($filename) < $namebuflen){
		my $nread = sysread(STDIN,
				    $filename,
				    $namebuflen - length($filename),
				    length($filename));
		die "read failed"
		    if ($nread < 0);
		$address += $nread;
#		print "read $nread\n";
	    }
	    $filename =~ s/\0*$//; # remove trailing nulls
	    $cramfs_i->{filename} = $currentPath.$filename;
	}else{

	    # no fname to read (root only!!)
	    $cramfs_i->{filename} = $currentPath;
	}
#	print Dumper($cramfs_i);
	my $debug = '';
#	$debug = '(offset %d : %d)';
	printf "%s %4d %4d %8d %s $debug\n",
	fancymodebits($cramfs_i->{mode}),
	$cramfs_i->{uid},
	$cramfs_i->{gid},
	$cramfs_i->{size},
	$cramfs_i->{filename},
	4*$cramfs_i->{offset}, $address;
	if (is_directory($cramfs_i->{mode})){
	    # insert entry in directory inode map
	    $dirOffsets->{4*$cramfs_i->{offset}} = $cramfs_i->{filename}. '/';
#	    print Dumper($dirOffsets);
	}else{
	    # insert entry into data file pointer map
	    $fileOffsets->{4*$cramfs_i->{offset}} =
	    {
	     name => $cramfs_i->{filename},
	     size => $cramfs_i->{size},
	     mode => $cramfs_i->{mode},
	     uid => $cramfs_i->{uid},
	     gid => $cramfs_i->{gid}
	     };
	}
#	print "Address $address\n";
	# check if we have moved to the first file in a new directory
	if (exists($dirOffsets->{$address})){
	    $currentPath = $dirOffsets->{$address};
	}
	# check if we have seen all the dir entries
	if (exists($fileOffsets->{$address})){
	    print "At $address file data for ",
	    $fileOffsets->{$address}->{name}, "\n";
	    exit 0;
	}
	
    }
}

	   # do it
main();



