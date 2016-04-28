#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

//AIMD values
#define USING_AIMD false
#define ADDITIVE_INCREASE 1.2
#define DECREASE_FACTOR .6

//Delay triggered values
#define DELAY_TRIGGER true
#define MAX_DELAY_THRESHOLD 80
#define MIN_DELAY_THRESHOLD 70
#define MIN_FULL_INCREASE 50

#define DELTA_DELAY false

#define DEFAULT_CWIND 10
#define DEFAULT_TIMEOUT 45
#define MIN_CWIND 2

#define EPSILON 3

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), cwind(DEFAULT_CWIND), lastDelay(-1), minDelay(10000),
  cwinds(), maxPower(0), bestCwind(0), bestDelay(0), maxDelayThreshold(0),
  minFullIncrease(0), minDelayThreshold(0)
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << cwind << endl;
  }

  return (int)cwind;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */
  cwinds.insert({sequence_number, window_size()});

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged datagram was sent (sender's clock) */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged datagram was received (receiver's clock)*/
			       const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{

  float increaseFactor = ADDITIVE_INCREASE;

  float delay = timestamp_ack_received - send_timestamp_acked;

  float dataSize = cwinds.at(sequence_number_acked);
  cwinds.erase(cwinds.find(sequence_number_acked));

  float power = dataSize / delay / delay;
  if (power > maxPower){
    maxPower = power;
    bestCwind = cwind;
    bestDelay = delay;
  }

  float diff = delay - lastDelay;
  float prediction = delay + cwind * diff;

  maxDelayThreshold = 2.2 * bestDelay;
  minFullIncrease = 1.75 * bestDelay;
  minDelayThreshold = 1.25 * bestDelay;  

  if (diff > EPSILON || prediction > maxDelayThreshold){
    increaseFactor *= lastDelay / delay * .25;
  }

  float scaledDelay = delay - minFullIncrease;
  if (delay > minFullIncrease && delay <= minDelayThreshold){
    increaseFactor *= (1 - scaledDelay / (maxDelayThreshold - minFullIncrease));
  }
  cerr << "maxPower: " << maxPower << endl << "bestCwind: " << bestCwind << endl << "bestDelay: " << bestDelay << endl;
  lastDelay = delay;

  if (DELAY_TRIGGER){
    if (delay < minDelayThreshold && prediction < maxDelayThreshold){
      cwind += (minDelayThreshold / delay) * increaseFactor / cwind;
    }
    else if (delay > maxDelayThreshold && cwind > MIN_CWIND){
      cwind -= .25;
    }
  }

  if (DELTA_DELAY && lastDelay > 0){
    
    if (diff > 0){
      cwind *= (diff / lastDelay);
    }
    else {
      cwind += - diff / cwind;
    }
    lastDelay = delay;
  }
  else if (lastDelay <= 0) {
    lastDelay = delay;
  }

  if (cwind < MIN_CWIND){
    cwind = MIN_CWIND;
  }

  /* Default: take no action */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

void Controller::timeout() {
  if (cwind > MIN_CWIND){
    //cwind *= DECREASE_FACTOR;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return DEFAULT_TIMEOUT;
}
