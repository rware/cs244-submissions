#!/bin/perl

# Array of candidate hashes,
# Each candidate hash has values for 
#   "signal_delay", "throughput", "power",
#    and each command line argument used
my @candidates = [];


# Sorts candidates by power, 
# only keeping the top 10 results
sub filter_candidates() {
    @candidates = (sort { $a->{'power'} <=> $b->{'power'} } @candidates)[0..10];
}

# "Mate" two candidates together,
# by averaging their parameter values
# and adding some amount of random variation
sub mate_candidates($$) {

}

sub gen_random_candidate() {
    
}

# Create a bunch of random candidates
# to begin with
sub seed_candidates() {

}

# Given a candidate, run the 
# test, recording signal delay, 
# throughput, and power from the test
sub run_candidate($) {

}
