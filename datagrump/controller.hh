#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>

enum Controller_State { SS, CA };
enum Controller_Mode { NA, AIMD, AIMD_INF, SIMPLE_DELAY, DOUBLE_THRESH };
/* Congestion controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */

  /* Add member variables here */
  unsigned int win_size_;
  unsigned int timeout_;
  unsigned int min_rtt_thresh_;
  unsigned int max_rtt_thresh_;
  uint64_t last_rtt_timestamp_;
  Controller_State state_;
  Controller_Mode mode_;

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

  /* A timeout was received */
  void timeout_received( void );

  /* How long to wait (in milliseconds) if there are no acks
     before sending one more datagram */
  unsigned int timeout_ms( void );
};

#endif
