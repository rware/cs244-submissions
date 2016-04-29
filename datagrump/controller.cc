#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

const int a = 1;
const float b = .5;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), cwnd_(12)
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
  if (timestamp_ack_received - send_timestamp_acked > 100) {
    cerr << "i: " << cwnd_ << endl;
    cwnd_ = max(1, int(static_cast<float>(cwnd_)*b));
    cerr << "o: " << cwnd_ << endl;
  } else {
    cwnd_ += a;
  }
  
  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

/* A timeout occurred */
void Controller::timeout_occurred() {
  cerr << "i: " << cwnd_ << endl;
  //cwnd_ = max(1, int(static_cast<float>(cwnd_)*b));
  cwnd_ = 1;
  cerr << "o: " << cwnd_ << endl;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 200;
}
