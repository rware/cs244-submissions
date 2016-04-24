#include <iostream>
#include <cmath>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

// TODO
static const double EWMA_WEIGHT = 0.8;
static const uint64_t T_LOW = 70;
static const uint64_t T_HIGH = 140;
static const uint64_t MIN_RTT = 30;
static const double ADDITIVE_INCREMENT = 0.5;
static const double MULTIPLICATIVE_DECREMENT = 0.5;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
  // , window_size_int(10)
  , rate_double(10)
  , in_timeout_batch(false)
  , prev_rtt(100) // TODO: initial value?
  , rtt_diff(10) // TODO: initial value?
  , hai_count(0)
{}

/* Get current window size, in datagrams */
unsigned int Controller::rate( void )
{
  // if ( debug_ ) {
  //   cerr << "At time " << timestamp_ms()
  //  << " window size is " << rate_int << endl;
  // }

  return rate_double;
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
  uint64_t new_rtt = timestamp_ack_received - send_timestamp_acked;
  int64_t new_rtt_diff = new_rtt - prev_rtt;
  cerr << "rtt diff is " << new_rtt_diff << endl;
  prev_rtt = new_rtt;
  rtt_diff = (1 - EWMA_WEIGHT)*rtt_diff + EWMA_WEIGHT*new_rtt_diff;
  double normalized_gradient = rtt_diff / MIN_RTT;

  if (new_rtt < T_LOW) {
    rate_double += 5 * ADDITIVE_INCREMENT;
    cerr << "below T_LOW, window size increasing to " << rate_double << endl;
  } else if (new_rtt > T_HIGH) {
    rate_double *= (1 - MULTIPLICATIVE_DECREMENT*(1 - T_HIGH/new_rtt));
    if (rate_double < 1) {
      rate_double = 1;
    }
    cerr << "above T_HIGH (rtt is " << new_rtt << "), window size decreasing to " << rate_double << endl;
  } else if (normalized_gradient <= 0) {
    rate_double += ADDITIVE_INCREMENT;
    cerr << "gradient is " << normalized_gradient << ", increasing window size to " << rate_double << endl;
  } else {
    rate_double *= (1 - MULTIPLICATIVE_DECREMENT*normalized_gradient);
    if (rate_double < 1) {
      rate_double = 1;
    }
    cerr << "gradient is " << normalized_gradient << ", decreasing window size to " << rate_double << endl;
  }

  // if (rate_double > 10) rate_double = 10;
  

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
  return 50;
}
