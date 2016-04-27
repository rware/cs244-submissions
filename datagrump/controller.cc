#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug, unsigned int delay_threshold, uint64_t max_packet_gap)
  : debug_( debug ),
    delay_threshold(delay_threshold),
    max_packet_gap(max_packet_gap)
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */

  uint64_t current_time = timestamp_ms();
  if((((int64_t)current_time) - ((int64_t)last_sent_datagram)) > 40) {
    //printf("long time since last sent packet: %ld\n", (((int64_t)current_time) - ((int64_t)last_sent_datagram)));
  }

  if((((int64_t)current_time) - ((int64_t)last_sent_datagram)) > (int64_t) max_packet_gap) {
    m_window_size++;
    last_sent_datagram = current_time;
    consecutive_decrease = 0;
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

  //printf("time since last sent packet: %ld\n", (((int64_t)timestamp_ack_received) - ((int64_t)last_sent_datagram)));

  /*if((((int64_t)timestamp_ack_received) - ((int64_t)last_sent_datagram)) > 40) {
    printf("long time since last sent packet: %ld\n", (((int64_t)timestamp_ack_received) - ((int64_t)last_sent_datagram)));
  }

  if((timestamp_ack_received - send_timestamp_acked) > delay_threshold) {
    if((((int64_t)timestamp_ack_received) - ((int64_t)last_sent_datagram)) < (int64_t) max_packet_gap && m_window_size > 1) {
      m_window_size--;
    }
  }
  else {
    m_window_size++;
  }*/

  if(timestamp_ack_received - send_timestamp_acked > delay_threshold) {
    if(m_window_size > 1) {
      if(consecutive_decrease == max_consecutive_decrease) {
        consecutive_decrease = 0;
      }
      else {
        consecutive_decrease++;
        m_window_size--;
      }
    }
  }
  else {
    consecutive_decrease = 0;
    m_window_size++;
  }
  //printf("Window Size: %u\n", m_window_size);
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 2; /* timeout of one second */
}
