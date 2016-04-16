#!/usr/bin/env perl

use warnings;
use strict;

chdir('..') or die "$!";
my @szs = (1, 2, 5, 10, 20, 50, 100, 200, 500, 1000);
local $^I = '';

print "size,thru,delay\n";

foreach my $sz (@szs) {
    local @ARGV = 'controller.cc';
    while (<>) {
        s/the_window_size = \d+;/the_window_size = ${sz};/g;
        print;
    }
    system('make 2>&1 > /dev/null');
    my $output = `./local-contest 2>&1`;
    print "$sz,";
    print "$1," if $output =~ m/Average throughput: (.*) Mbits/;
    print "$1\n" if $output =~ m/95th percentile signal delay: (.*) ms/;
}
