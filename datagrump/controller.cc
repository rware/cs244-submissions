#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

#define WINDOW_SIZE_FIXED 50
using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    win_size_( 1 ),
    max_rtt_thresh_( 100 ),
    mode_( SIMPLE_DELAY ),
    state_( SS )
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << win_size_ << endl;
  }

  if (mode_ == NA) {
    return (WINDOW_SIZE_FIXED);
  } else {
    return (win_size_);
  }
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
  unsigned int rtt;
  if (mode_ == AIMD) {
    win_size_++;
  } else if (mode_ == SIMPLE_DELAY) {
    rtt = timestamp_ack_received - send_timestamp_acked;
    if (rtt < max_rtt_thresh_) {
      win_size_++;
    } else {
      win_size_ = win_size_ * max_rtt_thresh_ / rtt;
      if (win_size_ == 0)
        win_size_ = 1;
    }
  }

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}


/* A timeout was received */
void Controller::timeout_received( void )
{
  if (mode_ == AIMD) {
    win_size_ = (win_size_ + 1) / 2;
  }
  return;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}
