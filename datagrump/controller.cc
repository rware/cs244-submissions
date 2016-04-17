#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

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
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
   << " window size is " << window_size_int << endl;
  }

  return window_size_int;
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
  if (timestamp_ack_received - send_timestamp_acked >= timeout_ms()) {
    if (!in_timeout_batch) {
      in_timeout_batch = true;
      // Multiplicative decrease
      window_size_double *= 0.5;
      window_size_int = static_cast<unsigned int>(window_size_double);
      if (window_size_int == 0) {
        window_size_int = 1;
      }
      if (debug_) {
        cerr << "timeout: window size is now " << window_size_int << endl;
      }
    }
  } else {
    in_timeout_batch = false;
    // Additive increase
    window_size_double += 1 / static_cast<double>(window_size_int);
    if (static_cast<unsigned int>(window_size_double) > window_size_int) {
      window_size_int = static_cast<unsigned int>(window_size_double);
      if (debug_) {
        cerr << "window size increased to " << window_size_int << endl;
      }
    }
  }

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
  return 200;
}
