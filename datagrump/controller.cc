#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug, unsigned int delay_threshold, uint64_t max_packet_gap)
  : debug_( debug ),
    delay_threshold(delay_threshold),
    max_packet_gap(max_packet_gap)
{
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */

  uint64_t current_time = timestamp_ms();

  //send one packet if we go more than max_packet_gap without sending one, regardless of congestion status
  if((((int64_t)current_time) - ((int64_t)last_sent_datagram)) > (int64_t) max_packet_gap) {
    m_window_size++;
    last_sent_datagram = current_time;
    decrease_counter = counter_denom - 1;
  }

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << m_window_size << endl;
  }

  return m_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }

  last_sent_datagram = send_timestamp;
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

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }



  int64_t delay = ((int64_t)timestamp_ack_received) - ((int64_t)send_timestamp_acked);

  if(delay < min_seen_delay)
    min_seen_delay = delay;

  if(ptindex == -1) {
    ptindex = 0;
    for(int i = 0; i < PACKET_TIMES; i++) {
      packet_times[i] = (double) delay;
    }
  }

  //calculate mean of last 10 packet times
  double mean = 0;
  for(int i = 0; i < PACKET_TIMES; i++) {
    mean += packet_times[i];
  }
  mean /= PACKET_TIMES;
  ptindex = (ptindex+1)%PACKET_TIMES;

  packet_times[ptindex] = (double) delay;


  //decrease window size only every other packet
  if(delay > mean+1 || delay > delay_threshold+min_seen_delay) {
    if(m_window_size > 1) {
      if((decrease_counter + counter_num)/counter_denom > decrease_counter/counter_denom) {
        m_window_size--;
      }
      decrease_counter += counter_num;
      increase_counter = 0;
    }
  }
  else {
    decrease_counter = counter_denom - 1;
    m_window_size += (increase_counter+counter2_num)/counter2_denom - increase_counter/counter2_denom;
    increase_counter += counter2_num;
  }


  //printf("mean %lf delay %ld window size %u\n", mean, delay, m_window_size);
  //printf("Window Size: %u\n", m_window_size);
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  //we have to reduce the timeout so that we can execute the code that limits the inter-packet gap to 55ms
  return 2; /* timeout of 2ms */
}
