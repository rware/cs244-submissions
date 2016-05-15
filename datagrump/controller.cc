#include <iostream>

#include "controller.hh"
#include "timestamp.hh"
#include <math.h>

#define ALG 4

using namespace std;

#if ALG == 3

#define ALPHA 0.5
#define BETA 0.2
#define LAMBDA 1

double old_rtt = 50.0;
double min_rtt = 1000.0;
double diff = 0.0;
int count = 0;

#elif ALG == 4

double ssthresh = 1000;
unsigned int stageVal = 0; // 0 slowstart, 1 congestion avoidance
unsigned int numTimeouts = 0;
double lastWindow = 0;
#endif


/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{}

// Start with the best window size that we found
double window_size_val = 13.0;

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << window_size_val << endl;
  }
  return (unsigned int) window_size_val;
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
  /* Default: take no action */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
    << " yo " << recv_timestamp_acked - send_timestamp_acked
	 << endl;
  }
#if ALG == 1
    // Simple AIMD
    if (timestamp_ack_received - send_timestamp_acked < timeout_ms()) {
        window_size_val += 1;
    }
    else {
        window_size_val /= 2;
        if (window_size_val == 0) {
            window_size_val = 1;
        }
    }
#elif ALG == 2
    //delay triggered
    uint64_t rtt = timestamp_ack_received - send_timestamp_acked;
    uint64_t t_low = 50; 
    uint64_t t_high = 70;
    if (rtt < t_low) {
       window_size_val += 1;
    }
    else if (rtt > t_high) {
    	window_size_val -= 1;
        if (window_size_val <= 0) {
            window_size_val = 1;
        } 
    }
    //cout << "rtt: " << rtt << " window: " << window_size_val << endl;
#elif ALG == 3
    //timely like
    double rtt = timestamp_ack_received - send_timestamp_acked;
    min_rtt = (rtt < min_rtt ) ? rtt : min_rtt;
    double t_low = 50.0;
    double t_high = 100.0;
    double new_diff = rtt - old_rtt;
    //cout << "old rtt: " << old_rtt << " old diff: " << diff;
    diff = (1 - ALPHA) * diff + ALPHA * new_diff;
    double norm_grad = diff / min_rtt;
    old_rtt = rtt;
    if (rtt < t_low) {
       window_size_val += LAMBDA * 2;
    }
    else if (rtt > t_high) {
        //cout << "old window: " << window_size_val;
        window_size_val *= (1.0 - BETA *( 1.0 - t_high/rtt));
        //cout << " new window: " << window_size_val << endl;
        if (window_size_val <= 0) {
            window_size_val = 1;
        }
    }
    else if (norm_grad <= 0) {
    	++count;
        int N = 1;
        if (count >= 5) {
            N = 5;
        }
        window_size_val = window_size_val + LAMBDA * N;
    } else {
    	count = 0;
        //cout << "old window: " << window_size_val;
	window_size_val *= (1.0 - BETA * norm_grad);
        //cout << " new window: " << window_size_val << endl;
        if (window_size_val <= 0) {
            window_size_val = 1;
        } 
    }
    //cout << "diff: " << diff << " rtt: " << rtt << " window: " << window_size_val << endl;

#elif ALG == 4
    // In slow start
    double rtt = timestamp_ack_received - send_timestamp_acked;
    if (stageVal == 0) {
        // Timeout happened
        if (rtt >= timeout_ms() && lastWindow == 0) {
            stageVal = 0;
            lastWindow = window_size_val;
            numTimeouts += 1;
            ssthresh = window_size_val / 2;
            window_size_val = ssthresh + 3;
        }
        else if (rtt >= timeout_ms() && lastWindow/1.5 <= numTimeouts && numTimeouts != 0) {
            numTimeouts = 0;
            lastWindow = 0;
        }
        else if (rtt >= timeout_ms()) {
            numTimeouts += 1;
        }
        // Move to congestion avoidance
        else if (window_size_val > ssthresh) {
            window_size_val += (1/window_size_val);
            stageVal = 1;
        }
        // Still in slow start
        else {
            window_size_val += 2;
        }
    }
    else {
        // Timeout happened
        if (rtt >= timeout_ms() && lastWindow == 0) {
            stageVal = 0;
            lastWindow = window_size_val;
            numTimeouts += 1;
            ssthresh = window_size_val / 2;
            window_size_val = ssthresh + 3;
        }
        else if (rtt >= timeout_ms() && lastWindow/1.5 <= numTimeouts && numTimeouts != 0) {
            numTimeouts = 0;
            lastWindow = 0;
        }
        else if (rtt >= timeout_ms()) {
            numTimeouts += 1;
        }
        // Still in congestion avoidance
        window_size_val += 1/window_size_val;
    }
    //cout << rtt << " " << stageVal << " " <<window_size_val <<  " " << lastWindow << " " << numTimeouts <<  " " << ssthresh << endl;

#endif 
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 90; /* timeout of 70 millisecond */
}
