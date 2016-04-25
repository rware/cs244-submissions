#include <iostream>
#include <cmath>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

// TODO
static const double EWMA_WEIGHT = 0.4;
static const uint64_t T_LOW = 50;
static const uint64_t T_HIGH = 140;
static const uint64_t MIN_RTT = 30;
static const double ADDITIVE_INCREMENT = 2;
static const double MULTIPLICATIVE_DECREMENT = 0.5;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
  // , window_size_int(10)
  , window_size_double(10)
  , timeout_batch(0)
  , prev_rtt(100) // TODO: initial value?
  , rtt_diff(10) // TODO: initial value?
  , hai_count(0)
  , timestamp_changed(0)
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  // if ( debug_ ) {
  //   cerr << "At time " << timestamp_ms()
  //  << " window size is " << window_size_int << endl;
  // }

  return window_size_double;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
            /* of the sent datagram */
            const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */

  // if ( debug_ ) {
  //   cerr << "At time " << send_timestamp
  //  << " sent datagram " << sequence_number << endl;
  // }
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
  if (now - timestamp_changed < 10 && new_rtt <= 1000) {
    return;
  }
  timestamp_changed = now;

  int64_t new_rtt_diff = new_rtt - prev_rtt;
  // cerr << "rtt diff is " << new_rtt_diff << endl;
  prev_rtt = new_rtt;
  rtt_diff = (1 - EWMA_WEIGHT)*rtt_diff + EWMA_WEIGHT*new_rtt_diff;
  double normalized_gradient = rtt_diff / MIN_RTT;
  hai_count = normalized_gradient < 0 ? hai_count+1 : 0;

  if (new_rtt <= T_HIGH) timeout_batch = 0;

  if (new_rtt < T_LOW) {
    window_size_double += 5 * ADDITIVE_INCREMENT / sqrt(window_size_double);
    // cerr << "below T_LOW, window size increasing to " << window_size_double << endl;
  } else if (new_rtt > T_HIGH) {
    if (new_rtt > 1000) {
      timeout_batch = 0;
      window_size_double = 1;
    } else {
      if (timeout_batch) timeout_batch--;
      else {
        timeout_batch = 3;
        window_size_double *= (1 - MULTIPLICATIVE_DECREMENT*(1 - T_HIGH/new_rtt));
        if (window_size_double < 1) {
          window_size_double = 1;
        }
      }
    }
    cerr << "above T_HIGH (rtt is " << new_rtt << "), window size decreasing to " << window_size_double << endl;
  } else if (normalized_gradient <= 0 && new_rtt < 100) {
    int n = hai_count >= 3 ? hai_count / 2 : 1;
    window_size_double += n * ADDITIVE_INCREMENT / window_size_double;
    // cerr << "gradient is " << normalized_gradient << ", increasing window size to " << window_size_double << endl;
  } else if (normalized_gradient > 0 && new_rtt > 90) {
    window_size_double *= (1 - MULTIPLICATIVE_DECREMENT*normalized_gradient*0.7 / sqrt(window_size_double));
    if (window_size_double < 1) {
      window_size_double = 1;
    }
    // cerr << "gradient is " << normalized_gradient << ", decreasing window size to " << window_size_double << endl;
  }
  

  // if ( debug_ ) {
  //   cerr << "At time " << timestamp_ack_received
  //  << " received ack for datagram " << sequence_number_acked
  //  << " original packet sent " << send_timestamp
  //  << " (send @ time " << send_timestamp_acked
  //  << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
  //  << endl;
  // }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 100;
}
