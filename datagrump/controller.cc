#include <iostream>

#include "controller.hh"
#include "timestamp.hh"
#include <stdlib.h>


using namespace std;

const int a = 1;
const float b = .5;

//const int history = 4;
const float alpha = .5;

unsigned long min_rtt = 200;

uint fractional(float frac) {
  return rand() < frac ? 1 : 0;
}

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), cwnd_(12), q({100, 100, 100, 100})
{}

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
  auto delta = timestamp_ack_received - send_timestamp_acked;
  min_rtt = min(min_rtt, delta);
  //cout << min_rtt << endl;
  q.push_back(delta);

  float target = 2.2;
  
  float lower = 1.1;
  float upper = 3.0;
  
  if (delta > upper * min_rtt) {
    cwnd_ = max<uint>(1, cwnd_-1);
  } else if (delta < lower * min_rtt) {
    cwnd_ += 1;
  } else {
    float bit = (delta / min_rtt) - target;
    if (bit > 0) {
      cwnd_ = max<uint>(1, cwnd_-fractional(.5 * (bit / (target - lower))));
    } else {
      cwnd_ += fractional(.5 * (-bit / (upper - target)));
    }
  }

  q.pop_front();
  
//   if (delta > 4*min_rtt) {
//     cwnd_ = max<uint>(1, cwnd_-1);
//   } else if (delta > 3*min_rtt) {
//     cwnd_ = max<uint>(1, cwnd_-fractional(.8));
//   } else if (delta > 2.5*min_rtt) {
//     //cwnd_ = max<uint>(1, cwnd_-fractional(.5));
//   } else if (delta < 1.1*min_rtt) {
//     cwnd_ += 1;
//   } else if (delta < 2*min_rtt) {
//     cwnd_ += fractional(.5);
//   }
//   int increases = ((q[1] - q[0]) > 0 ? 1 : -1) + ((q[2] - q[1]) > 0 ? 1 : -1) + ((q[3] - q[2]) > 0 ? 1 : -1);
//   if (increases == 3 || delta > 3*min_rtt) {
//     // consistently increasing; shrink window
//     //cwnd_ = max((double) 1, cwnd_*.8);
//     cwnd_ = max(1, cwnd_-1);
//   } else if (increases == -3 || delta < 1.1 * min_rtt) {
//     // consistently decreasing; bump window
//     cwnd_ += 1;
//   }
  
  
  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

/* A timeout occurred */
void Controller::timeout_occurred() {
  //cerr << "i: " << cwnd_ << endl;
  //cwnd_ = max(1, int(static_cast<float>(cwnd_)*b));
  //cwnd_ = max((float) 1, cwnd_*b);
  //cerr << "o: " << cwnd_ << endl;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 4*min_rtt;
}
