#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

//AIMD values
#define USING_AIMD false
#define ADDITIVE_INCREASE 1
#define DECREASE_FACTOR .75

//Delay triggered values
#define DELAY_TRIGGER true
#define MAX_DELAY_THRESHOLD 125
#define MIN_DELAY_THRESHOLD 60

#define DELTA_DELAY false

#define DEFAULT_CWIND 40
#define DEFAULT_TIMEOUT 45
#define MIN_CWIND 10

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), cwind(DEFAULT_CWIND), lastDelay(-1)
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
  if (USING_AIMD){
    cwind += ADDITIVE_INCREASE / cwind;
  }

  float delay = timestamp_ack_received - send_timestamp_acked;

  if (DELAY_TRIGGER){
    if (delay < MIN_DELAY_THRESHOLD){
      cwind += (MIN_DELAY_THRESHOLD / delay) * ADDITIVE_INCREASE / cwind;
    }
    else if (delay > MAX_DELAY_THRESHOLD && cwind > MIN_CWIND){
      cwind *= DECREASE_FACTOR * (MAX_DELAY_THRESHOLD / delay);
    }
  }

  if (DELTA_DELAY && lastDelay > 0){
    float diff = delay - lastDelay;
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
    cwind *= DECREASE_FACTOR;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return DEFAULT_TIMEOUT;
}
