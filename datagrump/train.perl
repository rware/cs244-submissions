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


sub gen_base_candidate() {
    # Start with unknown power, delay, throughput
    my $candidate = {'power' => 0,
                     'signal_delay' => 0,
                     'throughput' => 0 };
}

sub gen_random_candidate() {
    my $candidate = gen_base_candidate();    

    # TODO add params 
    $candidate->{'param1'} = int(rand(100));

    return $candidate;
}

# "Mate" two candidates together,
# by averaging their parameter values
# and adding some amount of random variation
sub mate_candidates($$) {
    my $(c1, c2) = @_;
    my $c2 = 
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
