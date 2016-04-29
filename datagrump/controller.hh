#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>

/* Congestion controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */

  double window_size_;
  uint64_t rtt_min_;

  /* Double exponential smoothing estimators */
  double capacity_est_a_;
  double capacity_est_b_;
  
  unsigned int epoch_packets_;
  bool slow_start_;

  /* EPIK parameters */
  const double ALPHA_CAPACITY = 0.41;
  const double BETA_CAPACITY = 0.2;
  const uint64_t DESIRED_RTT = 60;
  const uint64_t EPOCH = 58;
  const double DELTA = 0.45;

  void update_capacity_estimate ( const uint64_t event_timestamp );

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
