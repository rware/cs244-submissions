#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>
#define PACKET_TIMES 10

/* Congestion controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */

  /* Add member variables here */

public:
  /* Public interface for the congestion controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in sender.cc) */

  /* Default constructor */
  Controller( const bool debug, unsigned int delay_threshold, uint64_t max_packet_gap);

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

  unsigned int m_window_size = 12;

  unsigned int delay_threshold;

  uint64_t counter_num = 5;
  uint64_t counter_denom = 10;
  uint64_t counter2_num = 1;
  uint64_t counter2_denom = 1;
  uint64_t decrease_counter = counter_denom-1;
  uint64_t increase_counter = 0;
  uint64_t max_packet_gap;
  uint64_t last_sent_datagram = 0;
  int64_t min_seen_delay = 100000;

  double packet_times[PACKET_TIMES];
  int ptindex = -1;
};


#endif
