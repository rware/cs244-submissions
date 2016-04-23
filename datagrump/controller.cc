#include <iostream>
#include <math.h>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_(debug), rtt_estimate(100), the_window_size(1.0), num_packets_received(0), first_of_burst(0), curr_interarrival(0), burst_count(0), burst_timer(0)
{
  debug_ = false;
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */
  

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
  /* Default: take no action */
  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }
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
  if (num_packets_received <= 1) {
    rtt_estimate = rtt_estimate == 0 ? newRoundTripTime : (0.8 * rtt_estimate + 0.2 * newRoundTripTime);
    first_of_burst = recv_timestamp_acked;
    burst_timer = rtt_estimate;
    burst_count++;
    if (newRoundTripTime > 95) {
      the_window_size /= 2;
    } else {
      the_window_size += 2.0/window_size();
    }
  } else {
    if (true) {
      if (newRoundTripTime > 95) {
        the_window_size /= 2;
      } else {
        the_window_size += 2.0/window_size();
      }
    }
    // rtt_estimate = rtt_estimate == 0 ? newRoundTripTime : (0.8 * rtt_estimate + 0.2 * newRoundTripTime);
    if (recv_timestamp_acked <= first_of_burst + 95) {
      burst_count++;
    } else {
      // cerr << burst_count << " packets with recv_timestamp_acked of " << recv_timestamp_acked << " with estimated rtt of " << rtt_estimate << endl;
      the_window_size = 0.8 * burst_count;
      // cerr << burst_count << " " << the_window_size << " " << rtt_estimate << endl;
      burst_count = 1;
      burst_timer = rtt_estimate;
      first_of_burst = recv_timestamp_acked;
    }
  }


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
   << "gradient value is " << normalized_gradient
	 << endl;
  }
}
/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 2 * rtt_estimate; /* timeout of one second */
}
