#!/bin/perl

use strict;
use warnings;
use Data::Dumper::Simple;

# Array of candidate hashes,
# Each candidate hash has values for 
#   "signal_delay", "throughput", "power",
#    and each command line argument used
my @candidates;

# Sorts candidates by power, 
# only keeping the top 10 results
sub filter_candidates() {
    @candidates = (sort { $b->{'power'} <=> $a->{'power'} } @candidates)[0..10];
}

sub gen_base_candidate() {
    # Start with unknown power, delay, throughput
    my $candidate = {'power' => 0,
                     'signal_delay' => 0,
                     'throughput' => 0 };

    return $candidate;
}

sub gen_random_candidate() {
    my $candidate = gen_base_candidate();    

    # TODO add params 
    $candidate->{'param1'} = int(rand(100));

    return $candidate;
}

# "Mate" all candidates together,
# by averaging their parameter values
# and adding some amount of random variation
sub mate_candidates() {
    my $c1;
    my $c2;

    my $bound = scalar(@candidates);
    for(my $i = 0; $i < $bound; $i++) {
        for(my $j = $i+1; $j < $bound; $j++) {
            $c1 = $candidates[$i];
            $c2 = $candidates[$j];
            my $c = gen_base_candidate();
            $c->{'param1'} = (($c1->{'param1'} + $c2->{'param1'}) / 2.0) * ((90 + rand(20)) / 100.0);
            push(@candidates, $c);
        }
    }
}

# Create a bunch of random candidates
# to begin with
sub seed_candidates() {
    for(my $i = 0; $i < 20; $i++) {
        gen_random_candidate();
    }
}

# Given a candidate, run the 
# test, recording signal delay, 
# throughput, and power from the test
sub run_candidate($) {
    # Parse output matching:
    #     Average capacity: 5.04 Mbits/s
    #     Average throughput: 3.65 Mbits/s (72.3% utilization)
    #     95th percentile per-packet queueing delay: 57 ms
    #     95th percentile signal delay: 93 ms

    my $c = $_[0];
}



##### MAIN #####

# Generate some random candidates
for(my $i = 0; $i < 20; $i++) {
    my $c = gen_random_candidate();
    push(@candidates, $c);
}

mate_candidates();
