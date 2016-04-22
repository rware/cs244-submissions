#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug, float cwnd_sz)
  : debug_( debug )
  , cwnd_ (cwnd_sz)
  , avg_rtt_ (0.0)
  , rtt_samples_ (0)
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << cwnd_ << endl;
  }

  return cwnd_;
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
  uint64_t rtt = (timestamp_ack_received - send_timestamp_acked);
  avg_rtt_ = (avg_rtt_ * rtt_samples_ + rtt)/(rtt_samples_ + 1);
  rtt_samples_ += 1;
  /* Simple AIMD, when ACK received, increment cwnd_ by 1 / cwnd_ */
  if (rtt <= avg_rtt_) {
    cwnd_ += 1 / cwnd_;
  } else {
    cwnd_ -= 1 / cwnd_;
  }
  cerr << "avg_rtt = " << avg_rtt_ << endl
       << " cwnd = " << cwnd_ << endl;
  

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

void Controller::timeout_occured( void ) {
  /* Simple AIMD, decrement by 1/2 */
  // cwnd_ = cwnd_ / 2;

  cerr << "Timeout occured!" << endl
       << "avg_rtt = " << avg_rtt_ << endl
       << " cwnd = " << cwnd_ << endl;

} 

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}
