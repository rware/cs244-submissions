#include <iostream>

#include "controller.hh"
#include "timestamp.hh"
#include <cmath>

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), cond_window_size(100), queue_size(0), target_delay(80),
    last_receipt(0), queue_sizes(), LR_err(0), old_link_rate(0), error_gain(2)
    
{}


/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << cond_window_size << endl;
  }

  return cond_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  queue_size += 1;
  queue_sizes.push(queue_size);
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
  // Calculate link rate
  int delay = timestamp_ack_received - send_timestamp_acked; 
  double eff_q_size = queue_sizes.front() < 5 ? 5 : queue_sizes.front();
  double link_rate = eff_q_size / (double) delay;
  queue_sizes.pop();
  
  // Do derivate calculation
  if (timestamp_ack_received - last_receipt > 20){
    LR_err = 1000 * (link_rate - old_link_rate) /
    	     (timestamp_ack_received - last_receipt);
    old_link_rate = link_rate;
    last_receipt = timestamp_ack_received;
  }

  //Add correction
  int correction = (int) (error_gain * LR_err);
  if (LR_err > 0) {
    correction *= -1;
  }
  cond_window_size = link_rate * target_delay + correction;  
 
  queue_size -= 1;
  if (cond_window_size < 0) {
    cond_window_size = 0;
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
  return 60; /* 60 ms timeout */
}
