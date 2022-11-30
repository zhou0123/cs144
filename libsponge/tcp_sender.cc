#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) ,_retransmission_timeout(retx_timeout){}

uint64_t TCPSender::bytes_in_flight() const {  
    return _next_seqno - _send_base; 
    }

void TCPSender::fill_window() {
    if (_window_size==0 || _fin || (_stream.buffer_empty()&& _syn&& !_stream.input_ended()))
    {
        return;
    }

    if (_syn==false)
    {
        _syn=true;
        TCPSegment seg;
        seg.header().syn = true;
        seg.header().seqno = wrap(0,_isn);
        _next_seqno = 1;
        _window_size--;
        _segments_out.push(seg);
        _copy_segments_out.push(seg);
    }
    else if (_stream.eof())
    {
        TCPSegment seg;
        seg.header().fin = true;
        seg.header().seqno =wrap(_next_seqno,_isn);
        _next_seqno++;
        _window_size--;
        _fin =true;
        _segments_out.push(seg);
        _copy_segments_out.push(seg);
    }
    else
    {
        while (!_stream.buffer_empty() && _window_size >0)
        {
            
            TCPSegment seg;
            seg.header().seqno = wrap(_next_seqno,_isn);
            uint64_t length = (TCPConfig::MAX_PAYLOAD_SIZE < _window_size) ? TCPConfig::MAX_PAYLOAD_SIZE : _window_size;
            seg.payload() = _stream.read(length);
            _window_size-=seg.length_in_sequence_space();
            if (_window_size>0 && _stream.eof())
            {
                seg.header().fin = true;
                _fin = true;
                _window_size--;
            }
            _next_seqno += seg.length_in_sequence_space();
            _segments_out.push(seg);
            _copy_segments_out.push(seg);
            if (_fin) break;

        }

    }
    if  (!_timer_running)
    {
        _timer_running=true;
        _log_time = 0;
    }
}


// //! \param ackno The remote receiver's ackno (acknowledgment number)
// //! \param window_size The remote receiver's advertised window size
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t ackno_ = unwrap(ackno,_isn,_send_base);
    if (ackno_>_next_seqno) return false;
    if (ackno_ ==1 && _send_base ==0)
    {
        _send_base =1;
        _log_time =0;
        _copy_segments_out.pop();
        _retransmission_timeout = _initial_retransmission_timeout;
        _consecutive_retransmissions=0;
    }
    else if (ackno_ == _next_seqno && _copy_segments_out.size()==1 && _fin)
    {
        _send_base+=_copy_segments_out.front().length_in_sequence_space();
        _copy_segments_out.pop();
    }

    else if (!_copy_segments_out.empty() && ackno_>=_send_base + _copy_segments_out.front().length_in_sequence_space())
    {
        uint64_t seq = unwrap(_copy_segments_out.front().header().seqno,_isn,_send_base);
        uint64_t length = _copy_segments_out.front().length_in_sequence_space();

        while (ackno_>= (seq+length))
        {
            _send_base+=_copy_segments_out.front().length_in_sequence_space();
            _copy_segments_out.pop();
            if (_copy_segments_out.empty()) break;
            seq = unwrap(_copy_segments_out.front().header().seqno,_isn,_send_base);
            length = _copy_segments_out.front().length_in_sequence_space();

        }
        _retransmission_timeout = _initial_retransmission_timeout;
        _log_time=0;
        _consecutive_retransmissions=0;
    }

    if (bytes_in_flight()==0) _timer_running =false;
    else if (bytes_in_flight()>=window_size)
    {
        _window_size =0;
        return true;
    }
    if (window_size == 0) {
        _window_size = 1;
        _win_size_zero = true;
    } else {
        _window_size = window_size;
        _win_size_zero = false;
    }
    fill_window();
    return true;

 }
// // ! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    _log_time+=ms_since_last_tick;

    if (!_copy_segments_out.empty() &&_timer_running && _log_time >= _retransmission_timeout)
    {
        
        _segments_out.push(_copy_segments_out.front());
        _consecutive_retransmissions++;
        if (!_win_size_zero) _retransmission_timeout*=2;
        _log_time =0;
    }
 }


unsigned int TCPSender::consecutive_retransmissions() const { 
    return _consecutive_retransmissions;
 }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno,_isn);
    seg.payload()={};
    _segments_out.push(seg);

}
bool TCPSender::in_closed()
{
    return next_seqno_absolute() ==0;
}

bool TCPSender::in_syn_sent()
{
    return next_seqno_absolute()>0 && next_seqno_absolute() == bytes_in_flight();
}

bool TCPSender:: in_syn_acked()
{
    return (next_seqno_absolute()>bytes_in_flight() && !stream_in().eof()) ||(stream_in().eof()&& next_seqno_absolute()<stream_in().bytes_written()+2);
}

bool TCPSender::in_fin_sent()
{
    return stream_in().eof() && next_seqno_absolute() == (stream_in().bytes_written()+2) && bytes_in_flight()>0;
}

bool TCPSender::in_fin_acked()
{
    return stream_in().eof() && next_seqno_absolute() == (stream_in().bytes_written()+2) && bytes_in_flight()==0;
}
