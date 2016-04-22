#include <iostream>
#include <math.h>

#include "controller.hh"
#include "timestamp.hh"

#define INITIAL_WINDOW_SIZE 1
#define MULTIPLICATIVE_DECREASE_FACTOR 2
#define CONSEQUETIVE_INCREASES 5
#define TIMEOUT_VAL 200
#define RTT_THRESH 100
#define RTT_LOW_THRESH 40
#define RTT_HIGH_THRESH 200
#define ALPHA 0.2
#define BETA 0.5

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    curr_window_size(INITIAL_WINDOW_SIZE),
    prev_rtt( 0 ),
    rtt_diff ( 0 ),
    min_rtt ( 0 ),
    increase_counter ( 0 )
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << this->curr_window_size << endl;
  }

  return max((int) this->curr_window_size, 1);
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
  /* Default: take no action */
  uint64_t rtt = timestamp_ack_received - send_timestamp_acked;
  if (prev_rtt == 0) {
    prev_rtt = rtt;
  }
  
  if (min_rtt == 0 || rtt < min_rtt) {
    min_rtt = rtt;
  }

  if (rtt < RTT_LOW_THRESH) {
    increase_counter = 0;
    _additiveIncrease();
    return;
  }

  if (rtt > RTT_HIGH_THRESH) {
    increase_counter = 0;
    curr_window_size = curr_window_size * (1.0 - BETA * 
                                          (1.0 - (RTT_HIGH_THRESH / (double) rtt)));
    return;
  }

  int new_rtt_diff = (int) rtt - (int) prev_rtt;
  prev_rtt = rtt;
  rtt_diff = (1.0 - ALPHA) * rtt_diff + ALPHA * new_rtt_diff;
  double normalized_gradient = rtt_diff / min_rtt;
  if (normalized_gradient <= 0) {
    increase_counter++;
    _additiveIncrease();
  } else {
    increase_counter = 0;
    _multiplicativeDecrease(normalized_gradient);
  }

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
   << "gradient value is " << normalized_gradient
	 << endl;
  }
}

void Controller::_multiplicativeDecrease( double gradient )
{
  if (debug_) {
    cout << "decreasing window from " << this->curr_window_size << endl;
  }
  this->curr_window_size = this->curr_window_size * (1.0 - BETA * gradient);
}

void Controller::_additiveIncrease( void )
{
  if (debug_) {
    cout << "increasing window from " << this->curr_window_size << endl;
  }
  if (increase_counter != CONSEQUETIVE_INCREASES) {
    this->curr_window_size += 2.0 / this->window_size();
  } else {
    increase_counter = 0;
    this->curr_window_size += 5.0 / this->window_size();
  }
}

void Controller::timeout_occured( void )
{
  if (debug_) {
    cout << "Timeout occured with window size " << this->curr_window_size << endl;
  }
  this->_multiplicativeDecrease(1);
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return TIMEOUT_VAL; /* timeout of one second */
}
