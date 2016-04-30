#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

const int DELAY_THRESHOLD = 100;
const float mult_decrease = 0.65;
const float alpha = 0.9;
const float gamma = 0.1;
const float beta = 0.1;
const float delta = 0.01;
//const uint64_t t_low = 25;
//const uint64_t t_high = 75;

float abs(float a) {
  return (a < 0) ? -a : a;
}

float min(float a, float b) {
  return (a < 0) ? a : b;
}


float max(float a, float b) {
  return (a < b) ? b : a;
}



/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{

  if ( debug_ ) {
    cerr << "At time "         << timestamp_ms()
         << " window size is " << the_window_size 
         << endl;
  }

  return the_window_size < 1 ? 1 : the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
            /* of the sent datagram */
            const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */
  if ( debug_ ) {
    cerr << "At time "        << send_timestamp
         << " sent datagram " << sequence_number 
         << endl;
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
  // Reset retry counter for timeouts
  retries = 0;

  // Measure the RTT for the packet
  uint64_t delay = timestamp_ack_received - send_timestamp_acked;

  if (delay < min_rtt) min_rtt = delay;

  // Update our average RTT measurment for use in the t_high and t_low i
  // bounds calculations

  avg_rtt = (1 - alpha) * avg_rtt + alpha * delay;

  t_high = avg_rtt * 1.8;
  t_low = avg_rtt * 0.6;

  // update the rtt_diff for the gradient calculation

  int64_t new_rtt_diff = delay - prev_rtt;

  rtt_diff = (1 - alpha) * rtt_diff + alpha * new_rtt_diff;

  float gradient = rtt_diff / min_rtt;
  prev_rtt = delay;

  // Set our N scaling factor

  if ( gradient <= 0) {
    num_neg_gradients ++;
    if (num_neg_gradients == 5)
      N = num_neg_gradients / 2;
  } else {
    num_neg_gradients = 0;
    N = 1;
  }

  // Run the meat of the algorithm

  // Too Low!
  if (delay < t_low) {
    the_window_size += N * (float) min_rtt / delay;
  }
  // Too High!
  else if (delay > t_high) {
    the_window_size = the_window_size * (1 - beta * (1 - t_high / delay));
  }
  // Gradient-based window modification
  else if (gradient <= 0) {
    the_window_size = the_window_size + N * (float) min_rtt / delay;
  } else {
    the_window_size = the_window_size * (1 - beta * gradient);
  }

  // make sure the window size doesn't drop too low

  the_window_size = (the_window_size < 1) ? 1 : the_window_size;

  if ( debug_ ) {
    cerr << "At time "                    << timestamp_ack_received
         << " received ack for datagram " << sequence_number_acked
         << " (send @ time "              << send_timestamp_acked
         << ", received @ time "          << recv_timestamp_acked 
         << " by receiver's clock). "     << endl
         << "rtt_diff: "                  << rtt_diff 
         << ", min_rtt: "                 << min_rtt 
         << ", new_rtt_diff: "            << new_rtt_diff 
         << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more window */
unsigned int Controller::timeout_ms( void )
{
  return avg_rtt * 2 * ( 1.0 + (float) retries / 10.0);
}
