#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

const int DELAY_THRESHOLD = 100;
const float mult_decrease = 0.65;
const float alpha = 0.9;
const float gamma = 0.1;
const float beta = 0.1;
const float delta = 0.01;
//const uint64_t t_low = 25;
//const uint64_t t_high = 75;

float abs(float a) {
  return (a < 0) ? -a : a;
}

float min(float a, float b) {
  return (a < 0) ? a : b;
}


float max(float a, float b) {
  return (a < b) ? b : a;
}



/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{

  if ( debug_ ) {
    cerr << "At time "         << timestamp_ms()
         << " window size is " << the_window_size 
         << endl;
  }

  return the_window_size < 1 ? 1 : the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
            /* of the sent datagram */
            const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */
  if ( debug_ ) {
    cerr << "At time "        << send_timestamp
         << " sent datagram " << sequence_number 
         << endl;
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
  uint64_t delay = timestamp_ack_received - send_timestamp_acked;

  if (delay < min_rtt) min_rtt = delay;

  avg_rtt = (1 - alpha) * avg_rtt + alpha * delay;

  //if (abs(delay - t_low) < abs(delay - t_high)) {
  //  t_low = (1 - gamma) * t_low + gamma * delay;
  //  std::cerr << "t_low: " << t_low << endl;
  //} else {
  //  t_high = (1 - gamma) * t_high + gamma * delay;
  //  std::cerr << "t_high: " << t_high << endl;
  //}
  

  //if (delay < t_low)
  //  t_low -= 10 * gamma;
  //else
  //  t_low += gamma;

  //if (delay > t_high)
  //  t_high += 10 * gamma;
  //else
  //  t_high -= gamma;

  //float avg_delay;

  //if (!filled) {
  //  samples[num_samples] = delay;
  //  delay_sum += delay;
  //  num_samples++;
  //  avg_delay = delay_sum / num_samples;
  //  if (num_samples == NUM_SAMPLES) {
  //    num_samples = 0;
  //    filled = true;
  //  }
  //}
  //else {
  //  int prev_sample = samples[(num_samples + 1) % NUM_SAMPLES];

  //  delay_sum -= prev_sample;
  //  delay_sum += delay;
  //  samples[num_samples] = delay;
  //  avg_delay = delay_sum / NUM_SAMPLES;

  //  num_samples = (num_samples + 1) % NUM_SAMPLES;
  //}

  //t_high = avg_rtt * 1.8;
  //t_low = avg_rtt * 0.6;

  int64_t new_rtt_diff = delay - prev_rtt;

  rtt_diff = (1 - alpha) * rtt_diff + alpha * new_rtt_diff;

  if ( debug_ ) 
    cerr << "rtt_diff: "       << rtt_diff 
         << ", min_rtt: "      << min_rtt 
         << ", new_rtt_diff: " << new_rtt_diff 
         << endl;
  float gradient = rtt_diff / min_rtt;
  prev_rtt = delay;

  //std::cerr << "t_high: " << t_high 
  //          << " t_low: " << t_low 
  //          << " delay: " << delay 
  //          << " gradient: " << gradient 
  //          << " window: " << the_window_size 
  //          << endl;

  //if ( gradient <= 0) {
  //  num_neg_gradients ++;
  //  if (num_neg_gradients <= 20)
  //    N = num_neg_gradients / 4;
  //} else {
  //  num_neg_gradients = 0;
  //  N = 1;
  //}

  //if ( gradient >= 0) {
  //  num_pos_gradients ++;
  //  if (num_pos_gradients <= 5)
  //    M = num_pos_gradients;
  //} else {
  //  num_pos_gradients = 0;
  //  M = 1;
  //}

  //if (delay - avg_delay > avg_delay * 4) { 
  //  the_window_size = 0;
  //  cerr << "returning" << endl;
  //  return;
  //}

  if (delay < t_low) {
    the_window_size += N * (float) min_rtt / delay;
  }
  else if (delay > t_high) {
    the_window_size = the_window_size * (1 - beta * (1 - t_high / delay));
  }
  else if (gradient <= 0) {
    if ( debug_ ) 
      cerr << "neg gradient" << endl;
    the_window_size = the_window_size + N * (float) min_rtt / delay;
  } else {
    if ( debug_ ) 
      cerr << "pos gradient: " << gradient << endl;
    the_window_size = the_window_size * (1 - beta * gradient);
  }

  the_window_size = (the_window_size < 1) ? 1 : the_window_size;

  if ( debug_ ) {
    cerr << "At time "                    << timestamp_ack_received
         << " received ack for datagram " << sequence_number_acked
         << " (send @ time "              << send_timestamp_acked
         << ", received @ time "          << recv_timestamp_acked 
         << " by receiver's clock)"
         << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return avg_rtt * 2;
  //return 250; /* timeout of 0.5 seconds */
}
