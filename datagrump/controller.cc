#include <algorithm>
#include <iostream>
#include <cmath>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

static const double EWMA_WEIGHT = 0.4;
static const uint64_t T_LOW = 50;
static const uint64_t T_HIGH = 140;
static const uint64_t MIN_RTT = 30;
static const double ADDITIVE_INCREMENT = 1;
static const double ADDITIVE_DECREMENT = -1.3;
static const double MULTIPLICATIVE_DECREMENT = 0.4;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
  , window_size_double(10)
  , prev_rtt(100)
  , rtt_diff(10)
  , timestamp_changed(0)
  , timeout_batch(0)
  , increment_count(0)
  , decrement_count(0)
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
   << " window size is " << window_size_double << endl;
  }

  return window_size_double;
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
  uint64_t now = timestamp_ms();
  uint64_t new_rtt = timestamp_ack_received - send_timestamp_acked;
  if (now - timestamp_changed < 10) {
    return;
  }
  timestamp_changed = now;

  int64_t new_rtt_diff = new_rtt - prev_rtt;
  prev_rtt = new_rtt;
  rtt_diff = (1 - EWMA_WEIGHT)*rtt_diff + EWMA_WEIGHT*new_rtt_diff;
  double normalized_gradient = rtt_diff / MIN_RTT;

  if (new_rtt <= T_HIGH) timeout_batch = 0;

  // Increase aggressively (additive) below T_LOW
  if (new_rtt < T_LOW) {
    decrement_count = 0;
    increment_count += 1.5;
    window_size_double += increment_count * ADDITIVE_INCREMENT / sqrt(window_size_double);
  }

  // Decrease aggressively (multiplicative) above T_HIGH
  else if (new_rtt > T_HIGH) {
    increment_count = 0;
    if (new_rtt > 1000) {
      // When the RTT is over a second, just drop the window to 1
      decrement_count += 1.5;
      timeout_batch = 0;
      window_size_double = 1;
    } else {
      // Decrease aggressively if we're above T_HIGH and not in a timeout batch
      if (timeout_batch) {
        timeout_batch--;
      } else {
        decrement_count += 1.5;
        timeout_batch = sqrt(window_size_double);
        window_size_double *= (1 - MULTIPLICATIVE_DECREMENT*(1 - T_HIGH/new_rtt));
        if (window_size_double < 1) {
          window_size_double = 1;
        }
      }
    }
  }

  // Gradient-based additive increase if the RTT isn't already pretty good
  else if (normalized_gradient <= 0 && new_rtt < 100) {
    increment_count++;
    decrement_count = 0;
    int n = max(increment_count / 2, 1.0);
    window_size_double += n * ADDITIVE_INCREMENT / window_size_double;
  }

  // Gradient-based additive decrease
  else if (normalized_gradient > 0) {
    increment_count = 0;
    decrement_count++;
    int n = max(decrement_count / 2, 1.0);
    window_size_double += n * ADDITIVE_DECREMENT / sqrt(window_size_double);
    if (window_size_double < 1) {
      window_size_double = 1;
    }
  }  

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
   << " received ack for datagram " << sequence_number_acked
   << " original packet sent " << send_timestamp_acked
   << " (send @ time " << send_timestamp_acked
   << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
   << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 100;
}
