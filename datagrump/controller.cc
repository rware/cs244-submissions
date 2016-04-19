#include <iostream>
#include <algorithm>

#include "controller.hh"
#include "timestamp.hh"

#define ALPHA 1.0/(double)8
#define BETA 1.0/(double)4

using namespace std;



/* Default constructor */
Controller::Controller( const bool debug) :
  debug_( debug ),
  windowSize( 1 ),
  ssthresh(1 << 15),
  srtt ( 0 ),
  rttvar ( 0 ),
  timeout ( 250 ),
  outgoingPackets(deque<pair<uint64_t, uint64_t>>())
  {}



/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */
  unsigned int the_window_size = (unsigned int) this->windowSize;
  
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
    << " window size is " << the_window_size << endl;
  }
  
  return the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number, /* of the sent datagram */
                                   const uint64_t send_timestamp ) /* in milliseconds */
{
  outgoingPackets.push_back(make_pair(sequence_number, send_timestamp));
  auto oldest_packet = outgoingPackets.front();
  
  if (oldest_packet.second + timeout < send_timestamp) {
    windowSize = windowSize / 2 == 0 ? 1 : windowSize / 2;
    ssthresh = windowSize;
  }
  
  if ( debug_ ) {
    cerr << "At time " << send_timestamp
    << " sent datagram " << sequence_number << endl;
  }
}

/* EMWA of RTT samples is calculated accoring to RFC 2988 */
void Controller::update_rtt(int64_t diff ) {
  if (srtt == 0) {
    srtt = diff;
    rttvar = diff / 2;
  } else {
    rttvar = (1.0 - BETA)*rttvar + BETA * abs(srtt - diff);
    srtt = (1.0 - ALPHA)*srtt + ALPHA * diff;
  }
  timeout = 300 < srtt + 4 * rttvar ? srtt + 4 * rttvar : 300;
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked, /* what sequence number was acknowledged */
                              const uint64_t send_timestamp_acked, /* when the acknowledged datagram was sent (sender's clock) */
                              const uint64_t recv_timestamp_acked, /* when the acknowledged datagram was received (receiver's clock)*/
                              const uint64_t timestamp_ack_received ) /* when the ack was received (by sender) */
{
  bool acked = false;
  update_rtt(timestamp_ack_received - send_timestamp_acked);
  for (size_t i = 0; i < outgoingPackets.size(); i++) {
    auto sent_seqno = outgoingPackets.front();
    if (sent_seqno.first > sequence_number_acked)
      break;
    
    outgoingPackets.pop_front();
    acked = true;
    if (windowSize < ssthresh) {
      windowSize++;
    }
  }
  if (windowSize >= ssthresh && acked)
    windowSize += 1 / windowSize;
  
  
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
  return timeout; /* timeout of one second */
}
