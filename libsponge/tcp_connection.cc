#include "tcp_connection.hh"

#include <iostream>
using namespace std;
// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return {_sender.stream_in().remaining_capacity()}; }

size_t TCPConnection::bytes_in_flight() const { return {_sender.bytes_in_flight()}; }

size_t TCPConnection::unassembled_bytes() const { return {_receiver.unassembled_bytes()}; }

size_t TCPConnection::time_since_last_segment_received() const { return {_time_since_last_seg_received}; }

void TCPConnection::segment_received(const TCPSegment &seg) { 
    if (!_active) return;
    _time_since_last_seg_received =0;
    if (seg.header().rst)
    {
        if (_sender.in_syn_sent() && !seg.header().ack) return;
        rst_happen(false);
        return;      
    }

    if (_receiver.in_listen() && !seg.header().syn) return;
    if(_sender.in_syn_sent() && seg.header().ack && seg.payload().size()>0) return;

    bool send_empty =false;
    if (seg.header().ack && !_sender.ack_received(seg.header().ackno,seg.header().win))
    {
        if (seg.payload().size()!=0)
        {
            send_empty = true;
        }
    }

    if (!_receiver.segment_received(seg))
    {
        if (seg.payload().size()!=0)
        {
            send_empty = true;
        }
    }
    else{
        if (seg.header().syn)
        {
            if (_sender.in_closed())connect();
        }
    }

    if(seg.length_in_sequence_space()>0)
    {
        send_empty =true;
    }

    if(send_empty && _sender.segments_out().empty() && _receiver.ackno().has_value()) _sender.send_empty_segment();

    fill_and_update(false,false);

 }


bool TCPConnection::active() const { return {_active}; }

size_t TCPConnection::write(const string &data) {
    size_t len = _sender.stream_in().write(data);
    fill_and_update(false,false);
    return len;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    if (!_active) return;
    _time_since_last_seg_received+=ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if (_sender.consecutive_retransmissions()>TCPConfig::MAX_RETX_ATTEMPTS)
    {
        rst_happen(true);
    }

    fill_and_update(false,false);
 }

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
   fill_and_update(false,false);
}

void TCPConnection::connect() {
    fill_and_update(true,false);
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            rst_happen(true);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}



void TCPConnection::rst_happen(bool is_sending)
{
    _active = false;
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();

    if (is_sending)
    {
        if (_sender.segments_out().empty()) _sender.send_empty_segment();
        fill_and_update(false,true);
    }
}
void TCPConnection::fill_and_update(bool send_syn,bool send_rst)
{
    if (send_syn || !_receiver.in_listen())_sender.fill_window();
    if (!_sender.segments_out().empty())
    {
        int size = _sender.segments_out().size();
        TCPSegment seg = _sender.segments_out().front();
        WrappingInt32 value(0);
        if (_receiver.ackno().has_value()) value = _receiver.ackno().value();
        uint64_t win_size = _receiver.window_size();

        for (int i=0;i<size;i++)
        {
            seg = _sender.segments_out().front();

            if (_receiver.ackno().has_value())
            {
                seg.header().ack=true;
                seg.header().ackno = value;
                seg.header().win = win_size;
            }
            if (send_rst) seg.header().rst = true;
            _sender.segments_out().push(seg);
            _sender.segments_out().pop();
        }
       
        
    }
    clean_shutdown();
    // cout<<_fsm.segments_out().front().header().syn<<endl;
    // cout<<_fsm.segments_out().front().header().ack<<endl;
    // cout<<_fsm.segments_out().front().header().ackno<<endl;
    // cout<<_fsm.segments_out().front().header().seqno<<endl;
    // cout<<_fsm.segments_out().front().header().rst<<endl;
    // cout<<_fsm.segments_out().front().payload().size()<<endl;
    // cout<<_fsm.segments_out().size()<<endl;

    // cout<<"aaaaaaaaa"<<endl;
}

void TCPConnection::clean_shutdown()
{
    if (_receiver.in_fin_recv() && !_sender.stream_in().eof())
    {
        _linger_after_streams_finish = false;
    }

    if (_sender.in_fin_acked() &&_receiver.stream_out().input_ended() )
    {
        if (!_linger_after_streams_finish || _time_since_last_seg_received>=10*_cfg.rt_timeout) _active = false;
    }
}