#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_(debug), rtt_estimate(200), the_window_size(14), num_packets_received(0), rtt_total(0)
{
  debug_ = false;
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */
  

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
  }

  return the_window_size;
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
}

uint64_t diff_ms(timeval t1, timeval t2)
{
    return (uint64_t)(((t1.tv_sec - t2.tv_sec) * 1000000) + 
            (t1.tv_usec - t2.tv_usec))/1000;
}

void Controller::delay_aiad_unsmoothedRTT(const uint64_t sequence_number_acked,
			       const uint64_t send_timestamp_acked,
			       const uint64_t timestamp_ack_received ) {

  uint64_t newRoundTripTime = timestamp_ack_received - send_timestamp_acked;
  num_packets_received++;
  rtt_total += newRoundTripTime;
  bool firstPacket = num_packets_received == 1;
  if (firstPacket) {
    rtt_estimate = newRoundTripTime;
  } else {
    if (newRoundTripTime <= rtt_estimate) {
    	the_window_size++;
    } else if (newRoundTripTime > rtt_estimate) {
    	the_window_size--;
      if (the_window_size <= 1) {
        the_window_size = 1;
      }
    }
	rtt_estimate = rtt_total / (float)num_packets_received;	
  }
  if ( debug_ ) {
    cerr << endl << "The estimated rtt for datagram " << sequence_number_acked << " is " << newRoundTripTime << ". " << endl << "The new rtt estimate is " << rtt_estimate << "." << endl << endl;
  }
}

void Controller::delay_aimd_smoothedRTT( const uint64_t sequence_number_acked,
             const uint64_t send_timestamp_acked,
             const uint64_t timestamp_ack_received )
{
  uint64_t newRoundTripTime = timestamp_ack_received - send_timestamp_acked;
  num_packets_received++;
  rtt_total += newRoundTripTime;
  bool firstPacket = num_packets_received == 1;
  if (firstPacket) {
    rtt_estimate = newRoundTripTime;
  } else {
 	  bool shouldChangeWindow = num_packets_received % (the_window_size < 16 ? 1 : the_window_size/16) == 0;
 	  if (shouldChangeWindow) {
      if (newRoundTripTime > rtt_estimate) {
    		the_window_size = the_window_size <= 1 ? 1 : the_window_size - 1;
      } else {
      	the_window_size++;
      }
 	  }
  	rtt_estimate = rtt_total / (float)num_packets_received;	
  }
  if ( debug_ ) {
    cerr << endl << "The estimated rtt for datagram " << sequence_number_acked << " is " << newRoundTripTime << ". " << endl << "The new rtt estimate is " << rtt_estimate << "." << endl << endl;
  }
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
  delay_aimd_smoothedRTT(sequence_number_acked, send_timestamp_acked, timestamp_ack_received);
  

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return rtt_estimate; /* timeout of one second */
}
