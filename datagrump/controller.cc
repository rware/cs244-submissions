#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

const int64_t MAX_DELAY = 1 << 30;
const double ALPHA = 0.2;

/* Default constructor */
Controller::Controller( const bool debug ) 
  : debug_(debug) 
  , cwnd_(25)
  , min_owdelay_(MAX_DELAY)
  , new_qdelay_(0)
  , prev_qdelay_(0)
  , qdelay_diff_ewma_(0)
  , ack_num_(0)
  , timeout_(50)
  , rtt_ewma_(0)
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
      << " window size is " << cwnd_ << endl;
  }

  return (unsigned int) cwnd_;
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
			       /* when the acknowledged datagram was received (receiver's clock) */
			       const uint64_t timestamp_ack_received )
             /* when the ack was received (by sender) */
{
  // calculate EWMA of RTT and update timeout
  int64_t rtt = (int64_t) timestamp_ack_received
              - (int64_t) send_timestamp_acked;
  rtt_ewma_ = (1 - ALPHA) * rtt_ewma_ + ALPHA * rtt; 

  if (rtt_ewma_ > 150)
    timeout_ = rtt_ewma_;
  else
    timeout_ = 50;

  // calculate queuing delay and the difference between two consecutive ones
  int64_t owdelay = (int64_t) recv_timestamp_acked 
                  - (int64_t) send_timestamp_acked;
  if (owdelay < min_owdelay_)
    min_owdelay_ = owdelay;
  new_qdelay_ = owdelay - min_owdelay_;

  if (ack_num_ == 0)
    prev_qdelay_ = new_qdelay_;
  ack_num_++;

  double new_qdelay_diff = new_qdelay_ - prev_qdelay_;
  prev_qdelay_ = new_qdelay_;

  // calculate EWMA of queuing delay differences
  qdelay_diff_ewma_ = (1 - ALPHA) * qdelay_diff_ewma_ 
                    + ALPHA * new_qdelay_diff;

  // update cwnd
  if (new_qdelay_ < 5) {
    cwnd_ += 0.5;
  } else if (new_qdelay_ > 19) {
    cwnd_ -= 0.5;
  } 

  if (qdelay_diff_ewma_ <= 0)
    cwnd_ += 0.1;
  else
    cwnd_ -= 0.1;
    
  if (cwnd_ < 1) {
    cwnd_ = 1;
    if (rtt_ewma_ >= 500)
      cwnd_ = 0;
  } else if (cwnd_ > 90)
    cwnd_ = 90;


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
  return (unsigned int) timeout_;
}
