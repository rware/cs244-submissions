#include <iostream>
#include <algorithm>
#include <math.h>

#include "controller.hh"
#include "timestamp.hh"

#define AVG_MULT 0.95
#define TRACKED_PKTS 200

uint64_t ctr = 0;

uint64_t lastPackets[TRACKED_PKTS];

using namespace std;



/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), packetsUntilIncrease(0), curWinSize(10),
    lastSendTimestamp(0), packetsUntilDecrease(0),
    slowStartThreshold(5), minRTT(~0u), avgRTT(0),
    targetRTT(100), linkRateStartTime(0), curLinkRate(0),
    linkRateNumPackets(0), prevLinkRate(0)
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

uint64_t lastPacketTimestamp = 0;

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

   if (ackedRTT < minRTT) {
    minRTT = ackedRTT;
    targetRTT = 2 * minRTT;
  }
  // update statistics for link rate info
  if (linkRateStartTime == 0) {
    linkRateStartTime = timestamp_ack_received;
  } else {
    if (linkRateStartTime + 100 < timestamp_ack_received) {
      prevLinkRate = curLinkRate;
      curLinkRate = (linkRateNumPackets) * 12.0 / (timestamp_ack_received - linkRateStartTime);
      linkRateStartTime = timestamp_ack_received;
      linkRateNumPackets = 0;
      cout << "Link Rate: " << curLinkRate << "    Curwinsize: " << curWinSize << " RTT: " << avgRTT << endl;
    }
  }

  // Estimate num packets can be sent in next 50mS


  linkRateNumPackets++;


  if (ackedRTT < 60) {
    if (curLinkRate != 0 && prevLinkRate != 0) {
      double nextLinkRate = curLinkRate + (curLinkRate - prevLinkRate) / 100.0 * 30;
      double numPacketsCanBeSent = 70 * nextLinkRate / 12.0;
      curWinSize = max(numPacketsCanBeSent, 1.0);
    }
  } else {
    if (curLinkRate != 0 && prevLinkRate != 0) {
      double nextLinkRate = curLinkRate + (curLinkRate - prevLinkRate) / 100.0 * 30;
      double numPacketsCanBeSent = 50 * nextLinkRate / 12.0;
      curWinSize = max(numPacketsCanBeSent, 1.0);
    }
  }
  
 
  uint64_t packetIndex = ctr % TRACKED_PKTS;
  ctr++;
  lastPackets[packetIndex] = ackedRTT;
  
 
  if (avgRTT == 0) {
    avgRTT = ackedRTT;
    targetRTT = ackedRTT;
  } else {
    avgRTT = AVG_MULT * avgRTT + (1 - AVG_MULT) * ackedRTT;
    /*double changeFactor =  (avgRTT - targetRTT) * 0.005;
    if (avgRTT > targetRTT) {
      curWinSize -= changeFactor * sqrt(curWinSize);
    } else {
      curWinSize -= changeFactor / sqrt(curWinSize);
    }
    if (curWinSize < 1) curWinSize = 1;
   */
    // if (ctr > TRACKED_PKTS && packetIndex == 0) {
    //   minRTT = lastPackets[0];
    //   for (int i = 0; i < TRACKED_PKTS; i++) {
    //     if (lastPackets[i] < minRTT) minRTT = lastPackets[i];
    //   }
    //   targetRTT = 2 * minRTT;
    // }

    // Second attempt to compute current throughput

    if (ctr % 40 == 0) {
      //cout << targetRTT << "    " << avgRTT << "    " << curWinSize << endl;
    }
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
  return 40; /* timeout of one second */
}
