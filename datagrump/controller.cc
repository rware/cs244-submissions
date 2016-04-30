#include <iostream>
#include <math.h>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_(debug),
    the_window_size(1.0),
    num_packets_received(0),
    first_of_burst(0), 
    burst_count(1),
    num_packets_sent(0),
    last_queue_occ(-1),
    num_increase(0.0)
{
  debug_ = false;
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
  }

  return max((int)the_window_size, 1);
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
  num_packets_sent++;
}

uint64_t abs_(uint64_t first, uint64_t second) 
{
  if (first > second) {
    return first - second;
  } else {
    return second - first;
  }
}

uint64_t max_(uint64_t first, uint64_t second) 
{
  return first > second ? first : second;
}

uint64_t min_(uint64_t first, uint64_t second) 
{
  return first > second ? second : first;
}

void Controller::delay_aiad_unsmoothedRTT(const uint64_t sequence_number_acked,
             const uint64_t send_timestamp_acked,
             const uint64_t recv_timestamp_acked,
             const uint64_t timestamp_ack_received )
{
  uint64_t newRoundTripTime = timestamp_ack_received - send_timestamp_acked;
  num_packets_received++;

  int newBufferOcc = num_packets_sent - num_packets_received;
  if (num_packets_received == 1) {
    // first packet, so start a new burst.
    first_of_burst = recv_timestamp_acked;
    if (newRoundTripTime <= 200) {
      the_window_size += 2.0/window_size();  
    }
  } else {
    if (last_queue_occ < newBufferOcc) {
      // back off if the buffer gets too full
      the_window_size -= 5.0/window_size();
    } else {
      // keep probing the network during the burst period
      the_window_size += 3.0/window_size();
    }
    if (recv_timestamp_acked <= first_of_burst + 70) {
      burst_count++;
    } else {
      // end of burst
      // set the new window size to be a a little bit less than the measured value to avoid
      // overflowing the queue. Smooth the change out with the old window size.
      double new_window_size = 0.45 * the_window_size + 0.45 * burst_count;
      the_window_size = new_window_size;

      burst_count = 1;
      first_of_burst = recv_timestamp_acked;
    }
  }
  last_queue_occ = newBufferOcc;
  if (debug_) {
    cerr << sequence_number_acked << endl;
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
  /* Default: take no action */
  delay_aiad_unsmoothedRTT(sequence_number_acked, send_timestamp_acked, recv_timestamp_acked, timestamp_ack_received);

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

void Controller::timeout_( void )
{
  the_window_size -= 1.5/window_size();
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 150; /* timeout of one second */
}
