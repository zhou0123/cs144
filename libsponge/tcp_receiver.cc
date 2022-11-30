#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader tcpheader = seg.header();
    bool flag = false;
    if (!syn)
    {
        if (!tcpheader.syn) return false;
        syn = true;
        isn = tcpheader.seqno;
    }
    uint64_t ackno = _reassembler.stream_out().bytes_written()+1;
    uint64_t abs_seq = unwrap(tcpheader.seqno,isn,ackno);
    size_t index = abs_seq-1+static_cast<uint64_t>(tcpheader.syn);
    
    if(!(index>=_reassembler.stream_out().bytes_written()+window_size() || index+seg.payload().copy().length()<=_reassembler.stream_out().bytes_written()) ) flag=true;
    if (tcpheader.fin || tcpheader.syn) flag = true;
    if (flag) _reassembler.push_substring(seg.payload().copy(),index,tcpheader.fin);
    return flag;

}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (!syn)
    {
        return nullopt;
    }
    uint64_t ackno = _reassembler.stream_out().bytes_written()+1;
    if (_reassembler.stream_out().input_ended()) ackno++;
    return wrap(ackno,isn);
 }

size_t TCPReceiver::window_size() const { return {_capacity - _reassembler.stream_out().buffer_size()}; }

bool TCPReceiver::in_listen()
{
    return !ackno().has_value();
}
bool TCPReceiver::in_syn_recv()
{
    return ackno().has_value() && !stream_out().input_ended();
}

bool TCPReceiver::in_fin_recv()

{
    return stream_out().input_ended();
}