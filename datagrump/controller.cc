#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), cwnd( 50.0 ),
    estimated_rtt( 0.0 )
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */
  unsigned int the_window_size = 50;

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
  }

  //return the_window_size;
  return (unsigned int) cwnd;
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

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }

  /*
   * AIMD algorithm + Delay Trigger
   * https://en.wikipedia.org/wiki/Additive_increase/multiplicative_decrease
   */
  const uint64_t measured_rtt =
    timestamp_ack_received - send_timestamp_acked;

  if (estimated_rtt == 0) {
    // Initialization
    estimated_rtt = measured_rtt;
  }
  double alpha_ = 0.7;
  double new_rtt = (alpha_ * estimated_rtt) +
    ((1.0 - alpha_) * measured_rtt);

  //if (measured_rtt > 1.20 * new_rtt) {
  //if (measured_rtt > 150) { // this is the delay-trigger
  if (measured_rtt > 170) {
    cwnd = 1;
  } else if (new_rtt > 170) { // this is the delay-trigger
    // We detect congestion,
    // so multiplicatively decrease
    cwnd *= (4.0/5.0);
  } else {
    if (cwnd < 20) {
      cwnd += 1;
    } else {
      cwnd += 1.0 / cwnd;
    }
  }

  estimated_rtt = new_rtt;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 50; /* timeout of one second */
}
