/*
 * ==============================================================================
 *
 *       Filename:  memory_list-inl.h
 *        Created:  05/17/15 23:32:21
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */


#ifndef __MEMORY_LIST_H__
#error This file should only be included from alpha/memory_list.h
#endif

#include "memory_list.h"

#define MemoryListType MemoryList<T, typename std::enable_if<std::is_pod<T>::value && !std::is_pointer<T>::value>::type>

template<typename T>
std::unique_ptr<MemoryListType> MemoryListType::Create(char* buffer, SizeType size) {
    if (size < sizeof(Header)) {
        return nullptr;
    }
    std::unique_ptr<MemoryListType> m(new MemoryListType);
    m->header_ = reinterpret_cast<Header*>(buffer);
    m->header_->magic = kMagic;
    m->header_->size = 0;
    m->header_->buffer_size = size;
    m->header_->node_size = sizeof(T);
    m->header_->free_list = kInvalidNodeId;
    m->header_->free_area = (sizeof(Header) + sizeof(T) - 1) / sizeof(T);
    m->buffer_ = buffer;
    m->base_ = m->header_->free_area;
    return std::move(m);
}

template<typename T>
std::unique_ptr<MemoryListType> MemoryListType::Restore(char* buffer, SizeType size) {
    if (size < sizeof(Header)) {
        return nullptr;
    }
    auto header = reinterpret_cast<Header*>(buffer);
    if (header->magic != kMagic
            || header->buffer_size < size
            || header->node_size != sizeof(T)) {
        return nullptr;
    }
    auto header_slots = (sizeof(Header) + sizeof(T) - 1) / sizeof(T);
    auto slots = size / sizeof(T);
    if (header->free_list != kInvalidNodeId
            && (header->free_list < header_slots || header->free_list >= slots)) {
        return nullptr;
    }

    if (header->free_area < header_slots || header->free_area > slots) {
        return nullptr;
    }

    std::unique_ptr<MemoryListType> m(new MemoryListType);
    m->header_ = header;
    m->buffer_ = buffer;
    m->base_ = header_slots;

    return std::move(m);
}

template<typename T>
typename MemoryListType::NodeId MemoryListType::Allocate() {
    NodeId result;
    if (header_->free_list != kInvalidNodeId) {
        result = header_->free_list;
        header_->free_list = *NodeIdToNodeIdPtr(result);
    } else {
        CHECK(header_->free_area >= base_);
        if (header_->free_area - base_ == max_size()) {
            throw std::bad_alloc();
        } else {
            result = header_->free_area;
            ++header_->free_area;
        }
    }
    ++header_->size;
    return result;
}

template<typename T>
void MemoryListType::Deallocate(NodeId id) {
    *NodeIdToNodeIdPtr(id) = header_->free_list;
    header_->free_list = id;
    --header_->size;
}

template<typename T>
void MemoryListType::Clear() {
    header_->size = 0;
    header_->free_list = kInvalidNodeId;
    header_->free_area = (sizeof(Header) + sizeof(T) - 1) / sizeof(T);
}

template<typename T>
T* MemoryListType::Get(NodeId id) {
    return reinterpret_cast<T*>(NodeIdToAddress(id));
}

template<typename T>
const T* MemoryListType::Get(NodeId id) const {
    return reinterpret_cast<T*>(NodeIdToAddress(id));
}

template<typename T>
bool MemoryListType::empty() const {
    return header_->size == 0;
}

template<typename T>
typename MemoryListType::NodeId* MemoryListType::NodeIdToNodeIdPtr(NodeId id) {
    assert (id != kInvalidNodeId);
    return reinterpret_cast<NodeId*>(NodeIdToAddress(id));
}

template<typename T>
char* MemoryListType::NodeIdToAddress(NodeId id) {
    assert (id != kInvalidNodeId);
    return const_cast<char*>(buffer_) + id * sizeof(T);
}

template<typename T>
typename MemoryListType::SizeType MemoryListType::size() const {
    return header_->size;
}

template<typename T>
typename MemoryListType::SizeType MemoryListType::max_size() const {
    auto header_slots = (sizeof(Header) + sizeof(T) - 1) / sizeof(T);
    return header_->buffer_size / sizeof(T) - header_slots;
}

#undef MemoryListType
