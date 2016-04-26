#include <iostream>
#include <math.h>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* initial window size. */
#define WINDOW_INIT 60.0
/* AIMD Scheme : minimum window size. */
#define AIMD_MIN 2.0
/* AIMD Scheme : additive constant (> 0). */
#define AIMD_ADD 1.0
/* AIMD Scheme : multiplicative constant. */
#define AIMD_MULT 5.0
/* Halve window size on timeout */
#define TIMEOUT_MULT 0.8

#define TARGET_DELAY 70

#define AVG_GAIN 0.2
#define VAR_GAIN 0.4
#define VAR_MULT 2.0

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
  , cur_window_size( WINDOW_INIT )
  , avg_delay( -1.0 )
  , var_delay( -1.0 )
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
  if ( avg_delay < 0 ) {
    avg_delay = delay;
    var_delay = 0;
  }
  else {
      avg_delay = AVG_GAIN * avg_delay + (1 - AVG_GAIN) * delay;
      var_delay = VAR_GAIN * var_delay + (1 - VAR_GAIN) * fabs(delay - avg_delay);
  }

  if ( avg_delay + VAR_MULT * var_delay > TARGET_DELAY )
  {
    cur_window_size -= AIMD_MULT * (avg_delay / TARGET_DELAY) / floor( cur_window_size );
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
  double res = cur_window_size * TIMEOUT_MULT;
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
  return 25; /* timeout of 200ms */
}
