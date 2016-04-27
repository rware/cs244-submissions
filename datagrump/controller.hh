#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>

#define NUM_RTTS 10

/* Congestion controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */
  double cwnd;
  double slow_st_thresh;

  int acks_received;
  unsigned int rtts[NUM_RTTS];
  int index;
  uint64_t time_slice_start;
  double est_throughput;

  void update(uint64_t cur_time);
  void rtt_add(unsigned int rtt);
  unsigned int rtt_avg();


  /* Add member variables here */

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
