#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

#define PACKET_SIZE 1424 // this is hardcoded in the sender.cc

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), cwnd( 50.0 ),
    estimated_rtt( 0.0 )
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  unsigned int the_window_size = (unsigned int) cwnd;
  uint64_t outstanding_packets =
    ((bytes_sent - bytes_ackd) / PACKET_SIZE);
  /*
  if (outstanding_packets > 30) {
    return 1;
  }
  */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size
	 << " and outstanding packets is " << outstanding_packets << endl;
  }

  /*
  Attempt to limit window size
  if (the_window_size > 20) {
    return 20;
  }
  */

  if (the_window_size < 1) {
    return 1;
  }

  return the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /*
  last_sent_seqno = sequence_number;
  last_sent_timestamp = send_timestamp;
  */
  bytes_sent += PACKET_SIZE;

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

  /*
   * Try to quantify how much outstanding data
   * is on the network.
   */
  bytes_ackd += PACKET_SIZE;

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

  /**
   * The best_rtt is presumably the best rtt possible.
   * If we are too far from the best rtt, we penalize cwnd.
   * Increment cwnd if we are within best rtt by some
   * boundary.
   */
  if (measured_rtt < best_rtt) {
    best_rtt = measured_rtt;
  }

  if (measured_rtt - best_rtt > (0.5 * best_rtt)) {
    cwnd -= 0.5;
  } else {
    cwnd += 0.5;
  }

  /*
   * Estimated RTT is used for AIMD + Delay-Trigger
   */
  if (estimated_rtt == 0) {
    // Initialization
    estimated_rtt = measured_rtt;
  }

  double alpha_ = 0.7;
  double new_rtt = (alpha_ * estimated_rtt) +
    ((1.0 - alpha_) * measured_rtt);

  /*
  // AIMD + Delay-Trigger

  //if (measured_rtt > 1.20 * new_rtt) { // AIMD
  //if (measured_rtt > 150) { // Delay-Trigger
  if (measured_rtt > 120) {
    cwnd *= (1.0 / 2);
  } else if (measured_rtt > 110) { // AIMD
    cwnd *= (2.0 / 3.0);
  } else if (new_rtt > 100) { // Delay-Trigger
    // We detect congestion, so multiplicatively decrease
    cwnd *= (3.0/4.0);
  } else {
    if (cwnd < 25) { // acts as the SSTHRESHOLD
      cwnd += 1;
    } else {
      cwnd += 1.0 / cwnd;
    }
  }
  */

  if ( true ) {
    cerr << "Measured RTT: " << measured_rtt
	 << ", Estimated RTT: " << estimated_rtt
	 << ", CWND: " << cwnd << endl;
  }

  if (cwnd < 1) {
    // Lower bound to ensure window size >= 1
    // If cwnd < 0, delay starts increasing and can't capture
    // accurate RTT anymore.
    cwnd = 1;
  }

  estimated_rtt = new_rtt;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 30; /* timeout of one second */
}
