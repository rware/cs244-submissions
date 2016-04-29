#include <iostream>
#include <algorithm>

#include "controller.hh"
#include "timestamp.hh"

#define AVG_MULT 0.95

using namespace std;



/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), packetsUntilIncrease(0), curWinSize(10),
    lastSendTimestamp(0), packetsUntilDecrease(0),
    slowStartThreshold(5), minRTT(~0u), avgRTT(0)
{
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << curWinSize << endl;
  }

  return curWinSize;
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
  uint64_t ackedRTT = timestamp_ack_received - send_timestamp_acked;
  minRTT = min(minRTT, ackedRTT);
  if (avgRTT == 0) {
    avgRTT = ackedRTT;
  } else {
    double newAvgRTT = AVG_MULT * avgRTT + (1 - AVG_MULT) * ackedRTT;
    curWinSize += (100 - newAvgRTT) * 0.01;
    avgRTT = newAvgRTT;
    if (curWinSize < 1) curWinSize = 1;
    cout << curWinSize << endl;
  }
  /* AIMD */




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
  return 100; /* timeout of one second */
}
