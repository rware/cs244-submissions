#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

#define RTT_GUESS 70
#define CWND_DEFAULT 30
#define CWND_MIN 1
#define TIMEOUT 60
#define RTT_THRESH 110
#define SLOW_ST_THRESH 10
#define RTT_WEIGHT 0.2
#define RTT_GAIN_FACTOR 0.005
#define MULT_DECREASE_FACTOR 2.25
#define TIME_SLICE 10
using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), cwnd(CWND_DEFAULT), rtt_avg(RTT_GUESS), rtt_gain(0),
    curr_timeslice(), num_packets_in_timeslice(), up_count(0)
{
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << cwnd << endl;
  }

  return cwnd;
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

  unsigned int curr_rtt = (timestamp_ack_received - recv_timestamp_acked) +
      (recv_timestamp_acked - send_timestamp_acked);
  float old_avg = rtt_avg;
  float curr_weight = RTT_WEIGHT;//cwnd;
  rtt_avg = curr_weight * curr_rtt + (1 - curr_weight) * rtt_avg;
  if (num_packets_in_timeslice != 0 && curr_timeslice < timestamp_ack_received/TIME_SLICE) {
    if (rtt_gain < 0) {
      cwnd *= (1.0 + (rtt_gain/num_packets_in_timeslice) * RTT_GAIN_FACTOR);
    } 
    rtt_gain = 0;
    num_packets_in_timeslice = 0;
  }
  curr_timeslice = timestamp_ack_received/TIME_SLICE;
  num_packets_in_timeslice++;  
  if (rtt_avg > old_avg) {
    rtt_gain--;
  } else if (rtt_avg <= old_avg) {
    rtt_gain++;
  }

  if (rtt_avg > RTT_THRESH && cwnd <= SLOW_ST_THRESH) {
      cwnd = CWND_MIN;
  } else if (rtt_avg > old_avg && old_avg < RTT_THRESH) {
      if (rtt_avg > RTT_THRESH) {
          cwnd /= MULT_DECREASE_FACTOR;
      }
  } else {
    cwnd += 1/cwnd;
  }
   
  if (cwnd < CWND_MIN) cwnd = CWND_MIN;

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
  return TIMEOUT; /* timeout of one second */
}
