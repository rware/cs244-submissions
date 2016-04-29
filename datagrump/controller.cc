#include <algorithm>
#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

static uint64_t last_update_timestamp = 0;
static unsigned int num_epochs = 0;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    window_size_(1),
    rtt_min_(UINT64_MAX),
    capacity_est_a_(0),
    capacity_est_b_(0),
    epoch_packets_(0),
    slow_start_(true)
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << window_size_ << endl;
  }
  if (!slow_start_) {
    window_size_ = max(1.0, (capacity_est_a_ + capacity_est_b_) * DESIRED_RTT);
  } 
  
  return (unsigned int)window_size_;
}

void Controller::update_capacity_estimate(const uint64_t event_timestamp) {
  uint64_t elapsed = event_timestamp - last_update_timestamp;
  if (elapsed >= EPOCH) {
    double epoch_capacity = (double)(epoch_packets_) / EPOCH;
    double expected_capacity = window_size_ / rtt_min_;
    double diff = expected_capacity - epoch_capacity;

    if (!slow_start_) {
      /* Double exponential smoothing updates */
      ++num_epochs;      
      if(num_epochs == 1) {
	capacity_est_b_ = epoch_capacity - capacity_est_a_;
	capacity_est_a_ = epoch_capacity;
      } else {
	double prev_capacity_est_a_ = capacity_est_a_;
	capacity_est_a_ = ALPHA_CAPACITY * epoch_capacity +
	  (1 - ALPHA_CAPACITY) * (capacity_est_a_ + capacity_est_b_);
	capacity_est_b_ =
	  BETA_CAPACITY * (capacity_est_a_ - prev_capacity_est_a_) +
	  (1 - BETA_CAPACITY) * capacity_est_b_;       
      }
    } else {
      /* TCP Vegas style slow start */
      if (diff > DELTA) {
	capacity_est_a_ = epoch_capacity;
	slow_start_ = false;
      }
    }

    epoch_packets_ = 0;
    last_update_timestamp = event_timestamp;
  }
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }
  update_capacity_estimate(send_timestamp);
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
  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }

  uint64_t rtt = timestamp_ack_received - send_timestamp_acked;
  
  if (rtt < rtt_min_) {
    rtt_min_ = rtt;
  }

  if (slow_start_) {
    ++window_size_;
  }
  
  update_capacity_estimate(timestamp_ack_received);
  ++epoch_packets_; 
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 75; /* timeout of 75 ms */
}
