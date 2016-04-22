#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>
#include <map>

/* Congestion controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */
  unsigned int rtt_estimate;
  unsigned int the_window_size;
  uint64_t num_packets_received;
  uint64_t rtt_total;

  /* Add member variables here */
  void delay_aiad_unsmoothedRTT( const uint64_t sequence_number_acked,
             const uint64_t send_timestamp_acked,
             const uint64_t timestamp_ack_received );
  void delay_aimd_smoothedRTT( const uint64_t sequence_number_acked,
             const uint64_t send_timestamp_acked,
             const uint64_t timestamp_ack_received );

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
