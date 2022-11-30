#include "stream_reassembler.hh"
#include<iostream>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    
    node tmp(data,index);
    // if(tmp._data.size()==0) return;
    if (set_eof && eof_index<tmp.end_index)
    {
        if (tmp._data.size()!=0)tmp._data.erase(eof_index - tmp.index, eof_index - tmp.end_index);
        tmp.end_index = eof_index;
    }


    if (tmp.end_index<expect_index) return;

    if (tmp.index<expect_index)
    {
        tmp._data.erase(0,expect_index-tmp.index);
        tmp.index = expect_index;
    }

    size_t max_index = expect_index+_output.remaining_capacity();

    if (tmp.end_index>max_index)
    {
        tmp._data.erase(tmp._data.begin() + (max_index - tmp.index), tmp._data.end());
        tmp.end_index = tmp._data.size()+tmp.index;
    }
    else{
        if (eof)
        {
            eof_index = tmp.end_index;
            set_eof = true;
        }
    }
    for (auto iter = log.begin();iter!= log.end();)
    {
        if (tmp.index<iter->index && tmp.end_index>= iter->index && tmp.end_index<iter->end_index)
        {
            size_t deteled = tmp.end_index-iter->index;
            iter->_data.erase(0,deteled);
            iter->index +=deteled;
            break;
        }
        if (tmp.index>=iter->index && tmp.end_index<=iter->end_index)
        {
            tmp._data.clear();
            tmp.end_index =  tmp.index;
            break;
        }
        if (tmp.index>=iter->index && tmp.index<= iter->end_index && tmp.end_index >= iter->end_index)
        {
            size_t delete_size = iter->end_index - tmp.index;
            tmp._data.erase(0, delete_size);
            tmp.index = iter->end_index;
            iter++;
            continue;
        }
        if (tmp.index<=iter->index && tmp.end_index>= iter->end_index)
        {
            _unassembled_bytes-=iter->_data.size();
            iter = log.erase(iter);
            continue;
        }
        if (tmp.end_index <= iter->index) {
            break;
        }
        iter++;
    }

    if (tmp._data.size()!=0)
    {
        for (auto iter = log.begin();;iter++)
        {
            if (iter->index>=tmp.end_index || iter== log.end())
            {
                log.insert(iter,tmp);
                _unassembled_bytes+=tmp._data.size();
                break;
            }
        }
    }

    for (auto iter = log.begin();iter!=log.end();)
    {
        if (iter->index == expect_index)
        {
            size_t written = _output.write(iter->_data);
            // cout<<iter->_data.substr(0,5)<<endl;
            _unassembled_bytes -= written;
            expect_index+=written;
            log.erase(iter++);
            if (expect_index == eof_index && set_eof) {
                break;
            }
        }
        else
        {
            break;
        }
    }

    if (set_eof && expect_index == eof_index)
    {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return {_unassembled_bytes}; }

bool StreamReassembler::empty() const { return {log.empty()}; }
