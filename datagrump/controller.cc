#include <iostream>
#include <math.h>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* AIMD Scheme : initial window size. */
#define AIMD_MIN 3.0
/* AIMD Scheme : additive constant (> 0). */
#define AIMD_ADD 1.0
/* AIMD Scheme : multiplicative constant. */
#define AIMD_MULT 0.75

#define TARGET_DELAY 100

#define SMOOTHING_FACTOR 0.2

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
  , cur_window_size( AIMD_MIN )
  , avg_delay( -1.0 )
{ }

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */
  unsigned int the_window_size = static_cast<int>(cur_window_size);

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
  }

  return the_window_size;
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
  int delay = timestamp_ack_received - send_timestamp_acked;
  if ( avg_delay < 0 )
    avg_delay = delay;
  else
    avg_delay = (1.0 - SMOOTHING_FACTOR)*delay + SMOOTHING_FACTOR*avg_delay;
  if ( avg_delay > TARGET_DELAY )
  {
    cur_window_size -= AIMD_MULT * (avg_delay / TARGET_DELAY);
    if ( cur_window_size < AIMD_MIN )
      cur_window_size = AIMD_MIN;
  }
  else
    cur_window_size += (TARGET_DELAY / avg_delay) * AIMD_ADD / floor( cur_window_size );

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

/* What happens when a timeout is experienced. */
void Controller::timeout_experienced( void )
{
  /* AIMD Scheme : divide the window size. */
  double res = cur_window_size * AIMD_MULT;
  if ( res <= AIMD_MIN )
    cur_window_size = AIMD_MIN;
  else
    cur_window_size = res;

  int w_s = window_size (); //useful for debugging

  if ( debug_ )
    cerr << "Timeout experienced ! New size : " << w_s << endl;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 100; /* timeout of 200ms */
}
