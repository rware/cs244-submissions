#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>

#define NUM_SAMPLES 25

/* Congestion controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */

  /* Add member variables here */
  uint64_t min_rtt = 20; 
  uint64_t prev_rtt = 20;
  float t_low = 60;
  float t_high = 110;

  float rtt_diff = 0;
  uint8_t N = 1;
  uint8_t M = 1;
  uint8_t num_neg_gradients = 0;
  uint8_t num_pos_gradients = 0;

  /* Default: fixed window size of 100 outstanding datagrams */
  float the_window_size = 20;
  int num_samples = 0;
  float delay_sum = 0;
  float first_delay = 0;
  bool filled = false;

  float samples[NUM_SAMPLES];

public:
  /* Public interface for the congestion controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in sender.cc) */

  /* Default constructor */
  Controller( const bool debug );

  /* Get current window size, in datagrams */
  unsigned int window_size( void );

  /* A datagram was sent */
  void datagram_was_sent( const uint64_t sequence_number,
        const uint64_t send_timestamp );

  /* An ack was received */
  void ack_received( const uint64_t sequence_number_acked,
         const uint64_t send_timestamp_acked,
         const uint64_t recv_timestamp_acked,
         const uint64_t timestamp_ack_received );

  /* How long to wait (in milliseconds) if there are no acks
     before sending one more datagram */
  unsigned int timeout_ms( void );
};

#endif
