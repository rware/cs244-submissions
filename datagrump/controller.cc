#include <iostream>
#include <cmath>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

static const uint64_t INCREASE_THRESHOLD = 70;
static const uint64_t DECREASE_THRESHOLD = 120;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
  , window_size_int(10)
  , window_size_double(window_size_int)
  , in_timeout_batch(false)
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
  if (timestamp_ack_received - send_timestamp_acked >= DECREASE_THRESHOLD) {
    if (debug_) {
        cerr << "RTT " << timestamp_ack_received - send_timestamp_acked << ", decreasing window size " << window_size_double << endl;
    }
    if (window_size_double >= 1) {
        window_size_double -= 1 / sqrt(window_size_double);
    }
  } else if (timestamp_ack_received - send_timestamp_acked <= INCREASE_THRESHOLD) {
    if (debug_) {
        cerr << "RTT " << timestamp_ack_received - send_timestamp_acked << ", increasing window size " << window_size_double << endl;
    }
    window_size_double += 1 / window_size_double;
  } else {
    if (debug_) {
        cerr << "RTT " << timestamp_ack_received - send_timestamp_acked << ", doing nothing" << endl;
    }
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
  return 500;
}
