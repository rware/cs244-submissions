#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>
#include <queue>

/* Congestion controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */

  /* Add member variables here */
  int cond_window_size;  /* Congestion Window size */
  int queue_size;        /* Packets in queue */
  int target_delay;      /* Goal for delay */
  double last_receipt;   /* Previous calculation of LR deriv */

  std::queue <int> queue_sizes;  /* Holds queue size of packets */
  double LR_err;                 /* Derivative  of LR */
  double old_link_rate;          /* Previous link rate */
  double error_gain;             /* Gain on error */
 

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
