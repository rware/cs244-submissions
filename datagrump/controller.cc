#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

static const double ALPHA = 0.3;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    window_size_(25),
    last_acked_num_(0),
    rtt_estimate_(50),
    delay_thresh_(120),
    last_md_(3)
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << window_size_ << endl;
  }

  return (int) window_size_;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp,
				    /* in milliseconds */
				    bool from_timeout )
                                    /* is this a timeout-triggered resend */
{
  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number
     << " because of a timeout? " << (from_timeout ? "yes" : "no") << endl;
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
  uint64_t observed_rtt = timestamp_ack_received - send_timestamp_acked;
  rtt_estimate_ = ALPHA * rtt_estimate_ + (1 - ALPHA) * observed_rtt;
  double gradient = (rtt_estimate_ - observed_rtt) / rtt_estimate_;
  last_md_++;

  if ( sequence_number_acked > last_acked_num_) {
    last_acked_num_ = sequence_number_acked;
  } else {
      return; // duplicate ack
  }

  if (observed_rtt > delay_thresh_) {
    if ( last_md_ > 5 && gradient < 0.03 ) {
        window_size_ /= 2;
        last_md_ = 0;
      }
  } else {
    window_size_ += 1 / window_size_;
  }

  if (window_size_ < 1) { 
      window_size_ = 1;
  }

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 100;
}
