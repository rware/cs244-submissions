#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>
#include <deque>
#include <utility>

/* Congestion controller interface */
using namespace std;

class Controller
{
private:
	bool debug_; /* Enables debugging output */
  unsigned int windowSize;
  float windowGrowing;
  unsigned int ssthresh;
  deque<pair<uint64_t, uint64_t> > outgoingPackets;
  uint64_t receivedAckno;
  unsigned int ackCount;
  unsigned int timeout;
  deque<uint64_t> arrivalTimes;
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
