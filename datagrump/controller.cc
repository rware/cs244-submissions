#include <iostream>
#include <algorithm>
#include <vector>
#include <climits>
#include <time.h>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Pseudoalgorithm: keep track of oldest unacked packet... if the oldest 
 * unacked packet is older than 100ms... scale back as far as possible. 
 * while oldest unacked packet is less than 100ms old.. increase FAST!
 */

bool comp(const uint64_t& a, const uint64_t& b)
{
  return b < a ? true : false;
}

/* Default constructor */
Controller::Controller( const bool debug, float cwnd_sz)
  : debug_( debug )
  , cwnd_ (cwnd_sz)
  , avg_rtt_ (0.0)
  , rtt_samples_ (0)
  , last_scale_back_ (INT_MAX)
  , outstanding_acks_ ()
{
  make_heap(outstanding_acks_.begin(), outstanding_acks_.end(), comp);
}


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
   /* A packet is sent, add it to the heap */

 /*  cerr << outstanding_acks_.size() << endl
        << cwnd_ << endl;
*/

  /* Check if min value of heap is greater than now - 100ms */
  if (!outstanding_acks_.empty()) {
    uint64_t min = outstanding_acks_.at(0);

    timespec fifty_ms;
    fifty_ms.tv_sec = 0;
    fifty_ms.tv_nsec = (TIMEOUT_IN_MS * 0.5) * 1e6;

    //smooth updates only over every 12.5ms, at most
    bool should_scale_back = (send_timestamp > TIMEOUT_IN_MS*2) 
         && (last_scale_back_ + timestamp_ms_raw(fifty_ms)/4 < send_timestamp);

    if (send_timestamp - 8 * timestamp_ms_raw(fifty_ms) > min) {
      
      if (should_scale_back) {
      //  cerr << "[!] UBER: " << cwnd_ << endl;
        cwnd_ *= 0.5;
      }

      last_scale_back_ = send_timestamp;

    } else if (send_timestamp - 4 * timestamp_ms_raw(fifty_ms) > min) {
      
      if (should_scale_back) {
       // cerr << "[!] EXPLOSION: " << cwnd_ << endl;
        cwnd_ *= 0.8;
      }

      last_scale_back_ = send_timestamp;

    } else if (send_timestamp - 2 * timestamp_ms_raw(fifty_ms) > min) {
      
      if (should_scale_back) {
        //cerr << "[!] Major: " << cwnd_ << endl;
        cwnd_ *= 0.95;
      }

      last_scale_back_ = send_timestamp;

    } else if (send_timestamp - timestamp_ms_raw(fifty_ms) > min) {

      if (should_scale_back) {
       // cerr << "[!] Minor: " << cwnd_ << endl;
        cwnd_ *= 0.97;
      }

      last_scale_back_ = send_timestamp;

    } else {
      float delta = 0.1;
      // if (last_scale_back_ + timestamp_ms_raw(fifty_ms) < send_timestamp) {
      //    //no sign of delay! lets go.
      // //  cerr << "boosting" << endl;
      //    delta *= 2;
      // }
      cwnd_ = cwnd_ + delta;
    }

  } else {
    cerr << "none outstanding" << endl;
    cwnd_ += 1;
  }

  outstanding_acks_.push_back(send_timestamp);
  push_heap(outstanding_acks_.begin(), outstanding_acks_.end(), comp);

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
  vector<uint64_t>::iterator it =
    find (outstanding_acks_.begin(), outstanding_acks_.end(), send_timestamp_acked);

  //element not found, means that ack is duplicate. nothing to do!
  if (it == outstanding_acks_.end()) {

  } else {
    //remove the element from the heap and restore the heap properties.
    outstanding_acks_.erase(it);
    make_heap(outstanding_acks_.begin(), outstanding_acks_.end(), comp);
  }



/*

  uint64_t rtt = (timestamp_ack_received - send_timestamp_acked);
  avg_rtt_ = (avg_rtt_ * rtt_samples_ + rtt)/(rtt_samples_ + 1);
  rtt_samples_ += 1;
  Simple AIMD, when ACK received, increment cwnd_ by 1 / cwnd_ 

  if (rtt <= avg_rtt_) {
    cwnd_ += 1 / cwnd_;
  } else {
    cwnd_ -= 1 / cwnd_;
  }
  cerr << "avg_rtt = " << avg_rtt_ << endl
       << " cwnd = " << cwnd_ << endl;

       */
  
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
  /*
  cerr << "Timeout occured!" << endl
       << "avg_rtt = " << avg_rtt_ << endl
       << " cwnd = " << cwnd_ << endl;*/

} 

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return TIMEOUT_IN_MS; /* timeout of one second */
}
