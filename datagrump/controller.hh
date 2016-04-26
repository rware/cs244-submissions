#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>
#include <algorithm>
#include <vector>

#ifndef TIMEOUT_IN_MS
#define TIMEOUT_IN_MS 100
#endif

/* Congestion controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */
  float cwnd_;
  float avg_rtt_;
  int rtt_samples_;
  uint64_t last_scale_back_;
  std::vector<uint64_t> outstanding_acks_;

  /* Add member variables here */

public:
  /* Public interface for the congestion controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in sender.cc) */

  /* Default constructor */
  Controller( const bool debug, float cwnd_sz);

  /* Get current window size, in datagrams */
  unsigned int window_size( void );

  /* A datagram was sent */
  void datagram_was_sent( const uint64_t sequence_number,
			  const uint64_t send_timestamp );

  void timeout_occured( void );

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
