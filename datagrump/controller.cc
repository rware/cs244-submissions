#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;


/* Default constructor */
Controller::Controller( const bool debug,
		  float gain,
        	  unsigned int queue_limit,
 		  uint64_t queue_delay_target,
		  unsigned int update_freq,
		  unsigned int noise_filter,
		  unsigned int timeout_ms_pow)
  : debug_( debug ), window_size_pow( 10 ), add_inc( 1 ), mult_dec( .5 ), mult_dec_threshold( 60 ), goal_rtt ( 60 ), window_estimate( 0 ), window_estimate_std( 0 ), window_list(), delays(), received(), gain( gain ), queuing_delay_target( queue_delay_target ), base_delays(), current_delays(), queue_limit( queue_limit ), ack_num( 1 ), update_freq( update_freq ), noise_filter( noise_filter ), timeout_ms_pow( timeout_ms_pow ), base_delay( 0 )
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */
  unsigned int the_window_size = this->window_size_pow;

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

void Controller::update_base_delay(const uint64_t delay) {
  if (this->ack_num % this->update_freq == 0) {
    uint64_t min_base = std::min(this->base_delays.back(), delay);
    this->base_delays.pop_back();
    this->base_delays.push_back(min_base);
  } else {
    this->base_delays.push_back(delay);
    if (this->base_delays.size() > this->queue_limit) {
      this->base_delays.erase(this->base_delays.begin());
    }
  }
  this->ack_num++;
}

void Controller::update_current_delay(const uint64_t delay) {
  this->current_delays.push_back(delay);
  while (this->current_delays.size() > this->noise_filter) {
    this->current_delays.erase(this->current_delays.begin());
  }
}

uint64_t Controller::get_current_delay() {
  uint64_t min = this->current_delays.front();
  for (unsigned int i = 0; i < this->current_delays.size(); i++ ) {
    if (this->current_delays[i] < min) {
      min = this->current_delays[i];
    }
  }
  return min;
}

uint64_t Controller::get_base_delay() {
  uint64_t min = this->base_delays.front();
  for (unsigned int i = 0; i < this->base_delays.size(); i++ ) {
    if (this->base_delays[i] < min) {
      min = this->base_delays[i];
    }
  }
  return min;
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
  /* part b & c (which was tuning these parameters) */
  /*Additive inc */
  //this->window_size_pow = this->window_size_pow + this->add_inc;

  /* Multiplicative decrease */
  //if (this->mult_dec_threshold < timestamp_ack_received - send_timestamp_acked) {
  //  this->window_size_pow = (int) this->window_size_pow * this->mult_dec;
  //}


  /* part d */
  /* WINNING */
  // if our delay is bigger than the target delay
  if (this->noise_filter < timestamp_ack_received - send_timestamp_acked) {  //use noise filter as param here for easy access
    if (sequence_number_acked % 2 == 0) {
      this->window_size_pow = (int) this->window_size_pow - 1;
    }
  } else {
    this->window_size_pow = this->window_size_pow + 1;  
  }

  // never  let our window be 0
  if (this->window_size_pow == 0) {
    this->window_size_pow = 1;
  }
  
  /* ledbat */
  /* 
  uint64_t delay = timestamp_ack_received - send_timestamp_acked;
  if( debug_ ){
    cerr << "updating base" << endl;
  }
  this->update_base_delay(delay);
  if ( debug_ ) {
    cerr << "updating current" << endl;
  }
  this->update_current_delay(delay);
  if ( debug_ ) {
    cerr << delay << endl;
  }
  int queuing_delay = (int) this->get_current_delay() - (int) this->get_base_delay();
  if ( debug_ ) {
    cerr << "queuing_delay" << queuing_delay << endl;
  }
  int target_diff = this->queuing_delay_target - queuing_delay;
  if ( debug_ ) {
    cerr << "target_diff" << target_diff << endl;
  }

  this->window_size_pow += (int) this->gain * (target_diff * 1.0 / this->window_size_pow);
  if ( debug_ ) {
    cerr << this->window_size_pow << endl;
  }
  if (this->window_size_pow == 0 || this->window_size_pow > 1000) {
     if ( debug_ ) {
       cerr << "sent window to 1 !" << endl;
     }
    this->window_size_pow = 1;
  }
  */

  // A scheme based on trying to find the best window size
  /*
  uint64_t pp_delay = timestamp_ack_received - send_timestamp_acked;
  if (pp_delay <= this->goal_rtt) {
    //Add window size to a list
    this->window_list.push_back(this->window_size_pow);
    if (this->window_list.size() >= 5) {
      if (this->window_list.size() > 100) {
        this->window_list.erase(this->window_list.begin());
      }
      double sum = std::accumulate(this->window_list.begin(),
        this->window_list.end(), 0.0);
      double mean =  sum / this->window_list.size();
      double sq_sum = std::inner_product(this->window_list.begin(),
        this->window_list.end(), this->window_list.begin(), 0.0);
      double stddev = std::sqrt(sq_sum / this->window_list.size() - mean * mean);
      this->window_estimate = mean;
      this->window_estimate_std = stddev;
    }
  }  

  // Additive increase
  if (this->window_size_pow < this->window_estimate || this->window_estimate < 1) {
    this->window_size_pow = this->window_size_pow + this->add_inc;
  }

  // Multiplicative decrease
  if (this->mult_dec_threshold < pp_delay && this->window_estimate > 0) {
    this->window_size_pow = (int) this->window_estimate * this->mult_dec;
  }
  */
  
  //A scheme based on delay slope calculations
  /*
  uint64_t pp_delay = timestamp_ack_received - send_timestamp_acked;
  this->delays.push_back(pp_delay);
  this->received.push_back(timestamp_ack_received);
  if (this->delays.size() > 4) {
     this->delays.erase(this->delays.begin());
     this->received.erase(this->received.begin());
  }
  // Calculate the average slope of delays
  double slope = 0.0;
  for (uint64_t i = 0; i < this->delays.size() - 1; i++) {
    uint64_t e1 = this->delays[i];
    uint64_t e2 = this->delays[i + 1];
    uint64_t r1 = this->received[i];
    uint64_t r2 = this->received[i + 1];
    double e_slope = 0.0;
    if (r2 - r1 == 0) {
      r2 = r2 + 1;
    }
    if (r2 - r1 != 0) {
      e_slope = ((1500.0 / e2) - (1500.0 / e1)) / (r2 - r1);
    }
    slope += e_slope;
  }
  slope = slope / (this->delays.size() - 1);

  // Additive increase
  if (slope <= -0.05) {
    this->window_size_pow = this->window_size_pow + this->add_inc;
  }

  // Multiplicative decrease
  if ((slope >= 0.05 && pp_delay < this->mult_dec_threshold )) {
    this->window_size_pow = (int) this->window_size_pow * this->mult_dec;
    if (this->window_size_pow < 1) {
      this->window_size_pow = 1;
    }
  }
  */

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
  return this->timeout_ms_pow; /* timeout of one second */
}
