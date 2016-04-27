#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

#define RTT_GUESS 100
#define CWND_DEFAULT 15
#define TIMEOUT 500
#define RTT_THRESH 125
#define SLOW_ST_THRESH 15

#define TIME_SLICE_LEN 50
#define THROUGHPUT_GUESS ((double)15 / (double)100)
#define THROUGHPUT_EST_WEIGHT 0.3

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), cwnd(CWND_DEFAULT), slow_st_thresh(SLOW_ST_THRESH),
    acks_received(0), rtts(), index(0), time_slice_start(0),
    est_throughput(THROUGHPUT_GUESS)

{
  for (int i = 0; i < NUM_RTTS; i++) {
    rtts[i] = RTT_GUESS;
  }
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */
  cwnd = rtt_avg() * est_throughput;

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << cwnd << endl;
  }

  return cwnd;
}

void Controller::update(uint64_t cur_time) {
    if (cur_time > time_slice_start + TIME_SLICE_LEN) {
        double new_throughput = (double) acks_received / TIME_SLICE_LEN;
        est_throughput = new_throughput * THROUGHPUT_EST_WEIGHT 
            + (1.0 - THROUGHPUT_EST_WEIGHT) * est_throughput;
        
        time_slice_start += TIME_SLICE_LEN;
        acks_received = 0;
    }
}

void Controller::rtt_add(unsigned int rtt) {
    rtts[index] = rtt;
    index++;
    if (index == NUM_RTTS) index = 0;
}

unsigned int Controller::rtt_avg() {
    uint64_t total = 0;
    for (int i = 0; i < NUM_RTTS; i++) {
        total += rtts[i];
    }
    unsigned int avg = total/NUM_RTTS;
    return avg;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */
  update(send_timestamp);

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
  /* Default: take no action */
  unsigned int curr_rtt = (timestamp_ack_received - recv_timestamp_acked) +
      (recv_timestamp_acked - send_timestamp_acked);
  rtt_add(curr_rtt);
  update(timestamp_ack_received);
  acks_received++;

  cwnd += 1/cwnd;


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
