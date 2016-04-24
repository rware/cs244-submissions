#!/bin/perl

use strict;
use warnings;
use POSIX;
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
    $candidate->{'param1'} = int(rand(150));
    $candidate->{'param2'} = int(rand(150));
    $candidate->{'param3'} = int(rand(150));

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
            $c->{'param1'} = ceil( (($c1->{'param1'} + $c2->{'param1'}) / 2.0) * ((90 + rand(20)) / 100.0) );
            $c->{'param2'} = ceil( (($c1->{'param2'} + $c2->{'param2'}) / 2.0) * ((90 + rand(20)) / 100.0) );
            $c->{'param3'} = ceil( (($c1->{'param3'} + $c2->{'param3'}) / 2.0) * ((90 + rand(20)) / 100.0) );
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

    my $command = "./run-train $c->{'param1'} $c->{'param2'} $c->{'param3'} 2>&1";
    
    print "Runing $command...\n";

    my $output = `$command`;

    $output =~ m/throughput: ([\d\.]+) Mbit.*signal delay: (\d+) ms/;
    $c->{'throughput'} = $1;
    $c->{'signal_delay'} = $2;
    $c->{'power'} = $1/($2/1000);

    print "Throughput $1\n";
    print "Signal Delay$2\n\n";
}



##### MAIN #####

my $i;
my $c;

# Generate some random candidates
for($i = 0; $i < 2; $i++) {
    $c = gen_random_candidate();
    push(@candidates, $c);
}

# Main loop:
# run candidates that haven't been run,
# keep the best 10, mate them togethr and repeat
my $iter = 0;
while(1) {
    for($i = 0; $i < scalar(@candidates); $i++) {
        $c = $candidates[$i];
        if($c->{'power'} == 0) {
            run_candidate($c);
        }
    }

    filter_candidates();

    print "Iteration $iter results:\n";
    Dumper(@candidates);

    mate_candidates();
    
    $iter++;
}
