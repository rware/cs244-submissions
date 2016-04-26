#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>
#include <deque>
#include <vector>
#include <utility>
#include <mutex>

/* Congestion controller interface */
using namespace std;

class Controller
{
private:
	bool debug_; /* Enables debugging output */
  double windowSize;
  unsigned int ssthresh;
  int64_t srtt;
  int64_t rttvar;
  uint64_t timeout;
  
  deque<pair<uint64_t, uint64_t> > outgoingPackets;
  /* Add member variables here */
	
public:
	/* Public interface for the congestion controller */
	/* You can change these if you prefer, but will need to change
	 the call site as well (in sender.cc) */
  double distribution[256];
  uint64_t tslice_start;
  int64_t packets_sent_this_slice;
  int64_t packets_queued;
  
  void updatePDF(void);
  mutex mController;
  void normalize(double * pdf, size_t len);
  void applyBrownian(double * pdf, size_t len);
  
  double poissonProb(double sample_rate, int packet_counts);
  int guessLinkRate(double * pdf, size_t len, double threshold);
  unsigned int makeForecast(void);
  
	/* Default constructor */
	Controller( const bool debug );
	
	/* Get current window size, in datagrams */
	unsigned int window_size( void );
	
  void update_rtt( int64_t diff );
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
