#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <assert.h>
#include "controller.hh"
#include "timestamp.hh"
#include <boost/math/distributions/normal.hpp>
#include <boost/math/distributions/poisson.hpp>
#include <thread>
//#include <cmath>
#define ALPHA 1.0/(double)8
#define BETA 1.0/(double)4

#define VOLATILITY 200 //Packets per second per sqrt(second)
#define TIME_SLICE 20 //milliseconds
#define SAMPLE_SIZE 256
#define MAX_RATE 1280 //Packets
#define SAMPLE_WIDTH 5
#define FORECAST_WINDOW 5
#define TARGET_DELAY 100
#define TOLERANCE 5

using namespace std;



/* Default constructor */
Controller::Controller( const bool debug) :
  debug_( debug ),
  windowSize( 1 ),
  ssthresh(1 << 15),
  srtt ( 0 ),
  rttvar ( 0 ),
  timeout ( 250 ),
  outgoingPackets(deque<pair<uint64_t, uint64_t>>()),
  tslice_start(timestamp_ms()),
  packets_sent_this_slice(0),
  packets_queued(0),
  mController()
  {
    thread([this](){ this->updatePDF(); }).detach();
  }

size_t rate_to_idx(double rate) {
  if (rate >= MAX_RATE)
    return MAX_RATE - 1;
  if (rate < 0)
    return 0;
  
  return (size_t)(rate / SAMPLE_WIDTH);
}

using boost::math::cdf;

void Controller::applyBrownian(double * pdf, size_t len) {
  double stddev = VOLATILITY * (sqrt(0.01 * TIME_SLICE));

  boost::math::normal motion_dist(0, stddev);

  for (size_t i = 0; i < len; i++) {
    double link_rate = i * SAMPLE_WIDTH;
    for (size_t j = rate_to_idx(link_rate - 5 * stddev);
         j < rate_to_idx(link_rate + 5 * stddev); j++) {
      double delta = cdf(motion_dist, j * (SAMPLE_WIDTH + 1) - link_rate)
      - cdf(motion_dist, j * SAMPLE_WIDTH - link_rate);
      
      assert(!std::isnan(pdf[i]));
      assert(!std::isnan(j*(SAMPLE_WIDTH + 1)));
      assert(!std::isnan(cdf(motion_dist, j*(SAMPLE_WIDTH + 1) - link_rate)));
      assert(!std::isnan(cdf(motion_dist, j*SAMPLE_WIDTH - link_rate)));
      assert(!std::isnan(delta));
      assert(!std::isnan(pdf[i] * delta));
      
      pdf[i] += pdf[i] * delta;
    }
    
  }
}

void Controller::normalize(double * pdf, size_t len) {
  double sum = 0;
  for (size_t i = 0; i < len; i++) { sum += pdf[i]; }
  for (size_t i = 0; i < len; i++) { pdf[i] /= sum; }

}

double Controller::poissonProb(double sample_rate, int packet_counts) {
  if (sample_rate == 0) {
    //Magic
    return (packet_counts == 0);
  } else {
    return boost::math::pdf(boost::math::poisson(sample_rate), packet_counts);
  }
}

int Controller::guessLinkRate(double * pdf, size_t len, double threshold) {
  double sum = 0;
  for (size_t i = 0; i < len; i++) {
    sum += pdf[i];
    if (sum > threshold)
      return i * SAMPLE_WIDTH;
  }
  return MAX_RATE;
}


unsigned int Controller::makeForecast(void) {
  mController.lock();
  double future_pdf[SAMPLE_SIZE];
  memcpy(future_pdf, distribution, SAMPLE_SIZE*sizeof(double));
  unsigned int sum_packets = 0;
  for (size_t i = 0; i < FORECAST_WINDOW; i++) {
    applyBrownian(future_pdf, SAMPLE_SIZE);
    normalize(future_pdf, SAMPLE_SIZE);
    sum_packets += guessLinkRate(future_pdf, SAMPLE_SIZE, TOLERANCE);
  }
  mController.unlock();
  return sum_packets;
}

void Controller::updatePDF(void) {
  
  //Initialize as uniform
  for (int i = 0; i < SAMPLE_SIZE; i++)
    distribution[i] = 1.0 / SAMPLE_SIZE;
  
  
  while (true) {
    sleep(TIME_SLICE);
    
    mController.lock();
    //Apply brownian motion
    applyBrownian(distribution, SAMPLE_SIZE);
    
    for (int i = 0; i < SAMPLE_SIZE; i++) {
      distribution[i] *= poissonProb(i*SAMPLE_WIDTH * 0.01, packets_sent_this_slice);
    }
    packets_sent_this_slice = 0;
    
    //Normalize
    normalize(distribution, SAMPLE_SIZE);
    mController.unlock();
  }
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */
  unsigned int availableTput = makeForecast();
  int the_window_size = availableTput - packets_queued;
  if (the_window_size < 0)
    the_window_size = 0;
  
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
  mController.lock();
  outgoingPackets.push_back(make_pair(sequence_number, send_timestamp));
  packets_queued++;
  mController.unlock();
  
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
  timeout = 250 < srtt + 4 * rttvar ? srtt + 4 * rttvar : 250;
}
                    


/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked, /* what sequence number was acknowledged */
                              const uint64_t send_timestamp_acked, /* when the acknowledged datagram was sent (sender's clock) */
                              const uint64_t recv_timestamp_acked, /* when the acknowledged datagram was received (receiver's clock)*/
                              const uint64_t timestamp_ack_received ) /* when the ack was received (by sender) */
{
  mController.lock();
  for (size_t i = 0; i < outgoingPackets.size(); i++) {
    auto sent_seqno = outgoingPackets.front();
    if (sent_seqno.first > sequence_number_acked)
      break;
    
    outgoingPackets.pop_front();
    packets_sent_this_slice++;
    packets_queued--;
  }
  mController.unlock();


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
  return 250; /* timeout of one second */
}
