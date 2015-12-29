/*
 * =============================================================================
 *
 *       Filename:  IOBuffer.cc
 *        Created:  12/29/15 10:23:45
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "IOBuffer.h"

namespace alpha {
IOBuffer::IOBuffer() : data_(nullptr) {}

IOBuffer::IOBuffer(size_t size) { data_ = new char[size]; }

IOBuffer::IOBuffer(char* data) : data_(data) {}

IOBuffer::~IOBuffer() { delete[] data_; }

IOBufferWithSize::IOBufferWithSize(size_t size) : IOBuffer(size), size_(size) {}

IOBufferWithSize::IOBufferWithSize(char* data, size_t size)
    : IOBuffer(data), size_(size) {}

IOBufferWithSize::~IOBufferWithSize() = default;
}
