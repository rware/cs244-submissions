#include <iostream>
#include <math.h>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* initial window size. */
#define WINDOW_INIT 60.0
/* Minimum window size. */
#define MIN_WINDOW_SIZE 2.0
/* Increase constant constant. */
#define INC_COEFF 1.5
/* Decrease constant constant. */
#define DEC_COEFF 3.0
/* Decerase the window size by 10% on timeout */
#define TIMEOUT_MULT 0.9

/* Minimum timeout (the timeout won't be smaller than this). */
#define MIN_TIMEOUT 50

/* The target delay is computed from the minimum observed delay, and this
 * constant. */
#define TARGET_DELAY_COEFF 1.5

/* Constants used for the delay estimator.  */
#define AVG_GAIN 0.2
#define VAR_GAIN 0.4
#define VAR_MULT 2.0

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
  , cur_window_size( WINDOW_INIT )
  , avg_delay( -1.0 )
  , var_delay( -1.0 )
  , min_delay( -1 )
  , timeout ( MIN_TIMEOUT )
{ }

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
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
  uint64_t delay = timestamp_ack_received - send_timestamp_acked;
  if ( avg_delay < 0 ) {
    avg_delay = delay;
    var_delay = 0;
    min_delay = delay;
    if ( min_delay > MIN_TIMEOUT)
      timeout = min_delay;
    else
      timeout = MIN_TIMEOUT;
  }
  else {
      avg_delay = AVG_GAIN * avg_delay + (1 - AVG_GAIN) * delay;
      var_delay = VAR_GAIN * var_delay + (1 - VAR_GAIN) * fabs(delay - avg_delay);
      if( delay < min_delay )
      {
        min_delay = delay;
        if ( min_delay > MIN_TIMEOUT)
          timeout = min_delay;
        else
          timeout = MIN_TIMEOUT;
      }
  }
  /* The target delay is 1.5 times the minimum observed delay. */
  double target_delay = TARGET_DELAY_COEFF*min_delay;

  if ( avg_delay + VAR_MULT * var_delay > target_delay )
  {
    cur_window_size -= DEC_COEFF * (avg_delay / target_delay) / floor( cur_window_size );
    if ( cur_window_size < MIN_WINDOW_SIZE )
      cur_window_size = MIN_WINDOW_SIZE;
  }
  else
    cur_window_size += INC_COEFF * (target_delay / avg_delay) / floor( cur_window_size );

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
  if ( res <= MIN_WINDOW_SIZE )
    cur_window_size = MIN_WINDOW_SIZE;
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
  /* Computing the timeout : the timeout is always equal to the minimum
   * recorded delay, or 50ms if the minimum recorded delay is less than
   * 50ms.
   */
  return timeout;
}
