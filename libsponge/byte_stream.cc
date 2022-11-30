#include "byte_stream.hh"
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):_capacity(capacity){}

size_t ByteStream::write(const string &data) {
    size_t len = min(data.size(),_capacity-buffer.size());
    for (size_t i=0;i<len;i++)
    {
        buffer.push_back(data[i]);
    }
    _written+=len;
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t length = min(len,buffer.size());
    return string().assign(buffer.begin(),buffer.begin()+length);

}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    size_t length = min(len,buffer.size());
    _read+=length;
    buffer.erase(buffer.begin(),buffer.begin()+length);
 }

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string tmp = peek_output(len);
    pop_output(len);
    return tmp;
}

void ByteStream::end_input() {_end=true;}

bool ByteStream::input_ended() const { return {_end}; }

size_t ByteStream::buffer_size() const { return {buffer.size()}; }

bool ByteStream::buffer_empty() const { return {buffer.size()==0}; }

bool ByteStream::eof() const { return buffer_empty() and _end;; }

size_t ByteStream::bytes_written() const { return {_written}; }

size_t ByteStream::bytes_read() const { return {_read}; }

size_t ByteStream::remaining_capacity() const { return {_capacity-buffer.size()}; }
