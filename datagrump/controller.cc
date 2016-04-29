#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

const int DELAY_THRESHOLD = 120;
const float mult_decrease = 0.65;

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

  return the_window_size;
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
  /* Default: take no action */
  int delay = timestamp_ack_received - send_timestamp_acked;

  if (delay >= DELAY_THRESHOLD && timestamp_ack_received > next_loss_time){
    // Multiplicative Decrease
    the_window_size = the_window_size * mult_decrease;

    // Lowerbound the window size to 1
    the_window_size = the_window_size < 1 ? 1 : the_window_size;

    next_loss_time = timestamp_ack_received + delay;
  }
  else{
    // Additive Increase
    the_window_size = the_window_size + 2.0 / delay;
  }


  if ( debug_ ) {
    cerr << "At time "                    << timestamp_ack_received
         << " received ack for datagram " << sequence_number_acked
         << " (send @ time "              << send_timestamp_acked
         << ", received @ time "          << recv_timestamp_acked 
         << " by receiver's clock)"
         << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}
