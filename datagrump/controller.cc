#include <iostream>
#include <math.h>

#include "controller.hh"
#include "timestamp.hh"

#define INITIAL_WINDOW_SIZE 1
#define MULTIPLICATIVE_DECREASE_FACTOR 2
#define TIMEOUT_VAL 50

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    curr_window_size(INITIAL_WINDOW_SIZE)
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
  this->curr_window_size += 1.0 / this->window_size();

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

void Controller::timeout_occured( void)
{
  cout << "Timeout occured with window size " << this->curr_window_size << endl;
  this->curr_window_size /= MULTIPLICATIVE_DECREASE_FACTOR;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return TIMEOUT_VAL; /* timeout of one second */
}
