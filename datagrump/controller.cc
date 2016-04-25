#include <iostream>
#include <algorithm>
#include <vector>
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
  return a < b ? true : false;
}

/* Default constructor */
Controller::Controller( const bool debug, float cwnd_sz)
  : debug_( debug )
  , cwnd_ (cwnd_sz)
  , avg_rtt_ (0.0)
  , rtt_samples_ (0)
  , outstanding_acks ()
{
  make_heap(outstanding_acks.begin(), outstanding_acks.end(), comp);
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

 /*  cerr << outstanding_acks.size() << endl
        << cwnd_ << endl;
*/


  /* Check if min value of heap is greater than now - 100ms */
  if (!outstanding_acks.empty()) {
    uint64_t min = outstanding_acks.at(0);

    timespec one_hundred_ms;
    one_hundred_ms.tv_sec = 0;
    one_hundred_ms.tv_nsec = TIMEOUT_IN_MS * 1e6;

    if (send_timestamp - timestamp_ms_raw(one_hundred_ms) > min) {
      
      cerr << "Send timestamp is " << send_timestamp << endl;
      cerr << min << " " << endl;
      for (size_t i = 0; i < outstanding_acks.size(); i++) {
        if (outstanding_acks[i] < min) {
          cerr << "bug exists";
        }
      }
      cerr << endl;


      cerr << "Scaling back!" << endl;
      cwnd_ = cwnd_ / 2 + 1;
    } else {
      cwnd_ += 0.2;
    }

  } else {
    cwnd_ += 0.5;
  }

  outstanding_acks.push_back(send_timestamp);
  push_heap(outstanding_acks.begin(), outstanding_acks.end());

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
    find (outstanding_acks.begin(), outstanding_acks.end(), send_timestamp_acked);

  //element not found, means that ack is duplicate. nothing to do!
  if (it == outstanding_acks.end()) {

  } else {
    //remove the element from the heap and restore the heap properties.
    outstanding_acks.erase(it);
    make_heap(outstanding_acks.begin(), outstanding_acks.end());
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
