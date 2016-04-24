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
    for(my $i = 0; $i < scalar(@candidates); $i++) {
        my $c = $candidates[$i];
        if(!defined $c || $c->{'power'} == 0) {
            splice @candidates, $i, 1;
            $i--;
            print("Removed candidate\n");
        }
    }

    # Only keep top 4 candiates,
    # because after mating this will be 5 + 4! = 29 candidates
    @candidates = (sort { $b->{'power'} <=> $a->{'power'} } @candidates)[0..4];
}

sub gen_base_candidate() {
    # Start with unknown power, delay, throughput
    my $candidate = {'power' => 0,
                     'signal_delay' => 0,
                     'throughput' => 0,
                     'param1' => 0,
                     'param2' => 0,
                     'param3' => 0 };

    return $candidate;
}

sub gen_random_candidate() {
    my $candidate = gen_base_candidate();    

    # TODO add params 
    $candidate->{'param1'} = int(rand(200));
    $candidate->{'param2'} = int(rand(200));
    $candidate->{'param3'} = int(rand(200));

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

            if(!defined $c1 || !defined $c2 || $c1->{'power'} == 0 || $c2->{'power'} == 0) {
                next;
            }

            print "Mating $i and $j\n";

            my $c = gen_base_candidate();

            $c->{'param1'} = ceil( (($c1->{'param1'} + $c2->{'param1'}) / 2.0) * ((90 + rand(20)) / 100.0) );
            $c->{'param2'} = ceil( (($c1->{'param2'} + $c2->{'param2'}) / 2.0) * ((90 + rand(20)) / 100.0) );
            $c->{'param3'} = ceil( (($c1->{'param3'} + $c2->{'param3'}) / 2.0) * ((90 + rand(20)) / 100.0) );

            push(@candidates, $c);
        }
    }

    print "After mating, " . scalar(@candidates) . " candidates\n\n";
}

# Create a bunch of random candidates
# to begin with
sub seed_candidates() {
    for(my $i = 0; $i < 30; $i++) {
        my $c = gen_random_candidate();
        push(@candidates, $c);
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

    $output =~ m/throughput: ([\d\.]+) Mbit/;
    $c->{'throughput'} = $1;

    $output =~ m/signal delay: (\d+) ms/;
    $c->{'signal_delay'} = $1;

    $c->{'power'} = $c->{'throughput'} / ($c->{'signal_delay'} / 1000.0);

    print "Throughput: $c->{'throughput'}\n";
    print "Signal Delay: $c->{'signal_delay'}\n";
    print "Power: $c->{'power'}\n\n";
}



##### MAIN #####

my $i;
my $c;

# Generate some random candidates
seed_candidates();

# Main loop:
# run candidates that haven't been run,
# keep the best 10, mate them togethr and repeat
my $iter = 0;
while(1) {
    for($i = 0; $i < scalar(@candidates); $i++) {
        $c = $candidates[$i];
        print "Running candiate: \n";
        print Dumper($c);
        if($c->{'power'} == 0) {
            run_candidate($c);
        }
    }

    filter_candidates();

    print "Iteration $iter results:\n";
    Dumper(@candidates);

    mate_candidates();

    filter_candidates();
    
    $iter++;
}
