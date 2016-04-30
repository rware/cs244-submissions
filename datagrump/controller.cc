#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

//AIMD values
#define ADDITIVE_INCREASE 1.5

//Delay triggered values
#define DELAY_TRIGGER true
#define MAX_DELAY_THRESHOLD 80
#define MIN_DELAY_THRESHOLD 70
#define MIN_FULL_INCREASE 50

#define UPDATE_TIME 5000

#define DEFAULT_CWIND 10
#define DEFAULT_TIMEOUT 45000
#define MIN_CWIND 2

#define EPSILON 3

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), cwind(DEFAULT_CWIND), lastDelay(-1), minDelay(10000),
  cwinds(), maxPower(0), bestCwind(0), bestDelay(0), maxDelayThreshold(0),
  minFullIncrease(0), minDelayThreshold(0), newMaxPower(0), newBestDelay(0),
  newBestCwind(0), lastUpdated(0)
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << cwind << endl;
  }

  return (int)cwind;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */
  cwinds.insert({sequence_number, window_size()});

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

  float increaseFactor = ADDITIVE_INCREASE;

  float delay = timestamp_ack_received - send_timestamp_acked;

  //Find the cwind at the time when this packet was sent
  float dataSize = cwinds.at(sequence_number_acked);
  cwinds.erase(cwinds.find(sequence_number_acked));

  bool update = true;

  //The power over a given interval is packets/time^2
  float power = dataSize / delay / delay;
  
  //We are always trying to maximize power
  if (power > maxPower){
    maxPower = power;
    bestCwind = dataSize;
    bestDelay = delay;

    newMaxPower = 0;
    newBestDelay = 0;
    lastUpdated = timestamp_ack_received;
    update = false;
  }

  if (timestamp_ack_received - lastUpdated > UPDATE_TIME) {
    maxPower = newMaxPower;
    bestDelay = newBestDelay;
    bestCwind = newBestCwind;

    newMaxPower = 0;
    newBestDelay = 0;
    lastUpdated = timestamp_ack_received;
    update = false;
  }

  if (power > newMaxPower && update) {
    newMaxPower = power;
    newBestCwind = dataSize;
    newBestDelay = delay;
  }

  float diff = delay - lastDelay;
  float prediction = delay + cwind * diff;


  //heuristic values. In a perfect world we would find some non-heuristic method
  maxDelayThreshold = 2 * bestDelay;
  minFullIncrease = 1.7 * bestDelay;
  minDelayThreshold = 1.2 * bestDelay;  

  //If the delay is increasing such that we will have too high of delay, 
  //Do not increase as much
  if (diff > EPSILON || prediction > maxDelayThreshold){
    increaseFactor *= lastDelay / delay * .25;
  }

  //Slow down how quickly we are increasing cwind if the delay is getting high
  float scaledDelay = delay - minFullIncrease;
  if (delay > minFullIncrease && delay <= minDelayThreshold){
    increaseFactor *= (1 - scaledDelay / (maxDelayThreshold - minFullIncrease));
  }


  lastDelay = delay;

  if (DELAY_TRIGGER){
    if (delay < minDelayThreshold && prediction < maxDelayThreshold){
      //increase cwind faster if delay is low
      cwind += (minDelayThreshold / delay) * increaseFactor / cwind;
    }
    else if (delay > maxDelayThreshold && cwind > MIN_CWIND){
      //Decrease cwind to minimum over 4 RTTs.
      cwind -= .25;
    }
  }

  if (cwind < MIN_CWIND){
    cwind = MIN_CWIND;
  }


  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

void Controller::timeout() {
  //Do nothing on timeout.
  //We already have delay triggered decrease.
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  if (bestDelay == 0) {
    return DEFAULT_TIMEOUT;
  }

  return bestDelay;
}
