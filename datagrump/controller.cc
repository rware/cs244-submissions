#include <iostream>

#include "controller.hh"
#include "timestamp.hh"
#include <cmath>

#define WINDOW_SIZE_FIXED 50
#define MAX_WIN_SIZE 1000
#define RTT_ADJUST_INTERVAL 1

using namespace std;

#define TIMEOUT_RESET 100

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    win_size_( 1 ),
    timeout_( 60 ),
    min_rtt_thresh_( 50 ),
    max_rtt_thresh_( 70 ),
    last_rtt_timestamp_(0),
    state_( SS ),
    mode_( DOUBLE_THRESH ),
    outstanding_packets_( ),
    last_timeout_( 0 )
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << win_size_ << endl;
  }

  if (mode_ == NA) {
    return (WINDOW_SIZE_FIXED);
  } else {
    return (win_size_);
  }
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */

  /* Keep track of sent packets and their times,
   * for our own implementation of timeouts */
  if (mode_ == SIMPLE_DELAY || mode_ == AIMD) {
    struct SentPacket sent = {sequence_number, send_timestamp};
    outstanding_packets_.insert(sent);

    /* Check if a timeout should have triggered */
    if(is_timeout(send_timestamp) && send_timestamp - last_timeout_ > TIMEOUT_RESET) {
        timeout_received();
        last_timeout_ = send_timestamp;
    }
  } 

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }
}

/* Iterate through outstanding_packets_,
 * check if one or more has timed out */
bool Controller::is_timeout(uint64_t current_time) {
    bool timeout = 0;

    std::set<SentPacket>::iterator it;
    it = outstanding_packets_.begin();
    while (it != outstanding_packets_.end())
    {
       if(current_time - (*it).sent_time > timeout_  ||
             current_time - (*it).sent_time > timeout_ ) {
         cout << current_time << "| Packet " << (*it).sequence_number << " has timed out (send time " << (*it).sent_time << endl;
         it = outstanding_packets_.erase(it);
         timeout = 1;
       } else {
         ++it;
       }
    }

    return timeout;
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
  unsigned int rtt;
  rtt = timestamp_ack_received - send_timestamp_acked;
  if ((mode_ == AIMD) || (mode_ == AIMD_INF)) {
      /* Don't want to increase window size too quickly
       * when there's no timeouts for a while - indicative of
       * overshooting the network capacity */
     uint64_t time_since_timeout = timestamp_ack_received - last_timeout_;
     if((uint64_t)(rand() % 150) > time_since_timeout) {
        cout << "Window++" << endl;
        win_size_++;
     } else {
        cout << "Window Throttled" << endl;
     }
  } else if (mode_ == SIMPLE_DELAY) {
    if (rtt < max_rtt_thresh_) {
      win_size_++;
    } else {
      win_size_ = win_size_ * max_rtt_thresh_ / rtt;
      if (win_size_ == 0)
        win_size_ = 1;
    }
  } else if (mode_ == DOUBLE_THRESH) {
    if (timestamp_ack_received - last_rtt_timestamp_ > RTT_ADJUST_INTERVAL) {
      last_rtt_timestamp_ = timestamp_ack_received;
      if (rtt < min_rtt_thresh_) {
        win_size_ = win_size_ * min_rtt_thresh_ / rtt + 1;
      }
      else if (rtt < max_rtt_thresh_) {
        win_size_++;
      } else {
        win_size_ = win_size_ * max_rtt_thresh_ / rtt;
      }
      if (win_size_ == 0)
        win_size_ = 1;
      if (win_size_ > MAX_WIN_SIZE)
        win_size_ = MAX_WIN_SIZE;
    }
  }
  cout << "winsize: " << win_size_ << "rtt: " << rtt << endl;

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}


/* A timeout was received */
void Controller::timeout_received( void )
{
  if ((mode_ == AIMD) || (mode_ == AIMD_INF)) {
    win_size_ = (win_size_ + 1) / 2;
  }
  return;
}


/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return (timeout_); /* timeout of one second */
}
