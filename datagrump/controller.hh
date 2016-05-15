#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>
#include <vector>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <iterator>

/* Congestion controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */

  /* Add member variables here */
  unsigned int window_size_pow;
  unsigned int add_inc;
  double mult_dec;
  unsigned int mult_dec_threshold;
  unsigned int goal_rtt;
  double window_estimate;
  double window_estimate_std;
  std::vector<uint64_t> window_list;
  std::vector<uint64_t> delays;
  std::vector<uint64_t> received;

 float gain;
 uint64_t queuing_delay_target;
  std::vector<uint64_t> base_delays;
  std::vector<uint64_t> current_delays;
 unsigned int queue_limit;
 unsigned int ack_num;
 unsigned int update_freq;
 unsigned int noise_filter;
 unsigned int timeout_ms_pow;
 unsigned int base_delay;

public:
  /* Public interface for the congestion controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in sender.cc) */

  /* Default constructor */
  //Controller( const bool debug );

  Controller( const bool debug,
		  float gain,
        	  unsigned int queue_limit,
 		  uint64_t queue_delay_target,
		  unsigned int update_freq,
		  unsigned int noise_filter,
		  unsigned int timeout_ms_pow);
  /* Get current window size, in datagrams */
  unsigned int window_size( void );

  /* A datagram was sent */
  void datagram_was_sent( const uint64_t sequence_number,
			  const uint64_t send_timestamp );

  void update_base_delay(const uint64_t delay);
  
  void update_current_delay(const uint64_t delay);
  
  uint64_t get_current_delay();
  
  uint64_t get_base_delay();
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
