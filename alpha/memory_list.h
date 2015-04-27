/*
 * ==============================================================================
 *
 *       Filename:  memory_list.h
 *        Created:  04/26/15 15:14:23
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __MEMORY_LIST_H__
#define  __MEMORY_LIST_H__

#include <cstring>
#include <cassert>
#include <cstddef>
#include <memory>
#include <limits>
#include <type_traits>
#include "compiler.h"

namespace alpha {
    template<typename T, typename SizeType = int32_t, typename Enable = void>
    class MemoryList;

    template<typename T, typename SizeType>
    class MemoryList<T, SizeType, 
          typename std::enable_if< std::is_pod<T>::value && !std::is_pointer<T>::value>::type> {
        public:
            using NodeId = SizeType;
            static const NodeId kInvalidNodeId = -1;
            static std::unique_ptr<MemoryList> Create(char* start, SizeType size);
            static std::unique_ptr<MemoryList> Restore(char* start, SizeType size);
            NodeId Allocate();
            void Deallocate(NodeId);
            void Clear();
            T* Get(NodeId id);
            const T* Get(NodeId id) const;
            SizeType BufferSize() const;
            SizeType Size() const;
            SizeType Capacity() const;
            bool Empty() const;

        private:
            MemoryList() = default;
            DISABLE_COPY_ASSIGNMENT(MemoryList);
            struct Node {
                NodeId prev;
                NodeId next;
                T val;
            };
            static const SizeType kNodeSize = sizeof(Node);

            struct Header {
                int64_t magic;
                SizeType padding;
                SizeType elements;
                SizeType buffer_size;
                SizeType node_size;
                NodeId used;
                NodeId unused;
            };
            static const SizeType kHeaderSize = sizeof(Header);
            static const SizeType kMinBufferSize = kHeaderSize;
            static const int64_t kMagic = 0x434da4cca9fea893;

            void InitHeader(char* start, SizeType size);
            bool RestoreHeader(char* start, SizeType size, SizeType* capacity);
            void Populate();
            void AddNodeToUsed(NodeId node_id);
            void AddNodeToUnused(NodeId node_id);
            void PrependNode(NodeId node, NodeId header);
            NodeId UnlinkNode(NodeId node_id);
            Node* GetNodeById(NodeId node_id);
            const Node* GetNodeById(NodeId node_id) const;

            void* start_;
            void* end_;
            void* data_start_;
            Header* header_;
            SizeType capacity_;

            friend class MemoryListUnittest;
            static_assert (std::is_signed<SizeType>::value, "SizeType must be signed type");
            static_assert (std::is_pod<Node>::value, "Node must be pod type");
            static_assert (std::is_pod<Header>::value, "Header must be pod type");
    };
#define MemoryListType MemoryList<T, SizeType, typename std::enable_if<std::is_pod<T>::value && !std::is_pointer<T>::value>::type>

    template<typename T, typename SizeType>
    std::unique_ptr<MemoryListType> MemoryListType::Create(char* start, SizeType size) {
        if (size < kMinBufferSize) {
            return nullptr;
        }
        std::unique_ptr<MemoryListType> l(new MemoryListType);
        l->start_ = start;
        l->end_ = start + size;
        l->data_start_ = start + kHeaderSize;
        l->InitHeader(start, size);
        l->Populate();
        return std::move(l);
    }

    template<typename T, typename SizeType>
    std::unique_ptr<MemoryListType> MemoryListType::Restore(char* start, SizeType size) {
        if (size < kMinBufferSize) {
            return nullptr;
        }
        std::unique_ptr<MemoryListType> l(new MemoryListType);
        l->start_ = start;
        l->end_ = start + size;
        l->data_start_ = start + kHeaderSize;
        if (l->RestoreHeader(start, size, &l->capacity_) == false) {
            return nullptr;
        }
        return std::move(l);
    }

    template<typename T, typename SizeType>
    typename MemoryListType::NodeId MemoryListType::Allocate() {
        auto res = header_->unused;
        if (likely(res != kInvalidNodeId)) {
            header_->unused = UnlinkNode(res);
            AddNodeToUsed(res);
            ++header_->elements;
        } else {
            throw std::bad_alloc();
        }
        return res;
    }

    template<typename T, typename SizeType>
    void MemoryListType::Deallocate(MemoryListType::NodeId node_id) {
        assert (node_id != kInvalidNodeId);
        auto next = UnlinkNode(node_id);
        if (node_id == header_->used) {
            header_->used = next;
        }
        AddNodeToUnused(node_id);
        assert (header_->elements > 0);
        --header_->elements;
    }

    template<typename T, typename SizeType>
    void MemoryListType::Clear() {
        Populate();
        header_->elements = 0;
    }

    template<typename T, typename SizeType>
    T* MemoryListType::Get(NodeId id) {
        auto node = GetNodeById(id);
        return node ? &node->val : nullptr;
    }

    template<typename T, typename SizeType>
    const T* MemoryListType::Get(NodeId id) const {
        auto node = GetNodeById(id);
        return node ? &node->val : nullptr;
    }

    template<typename T, typename SizeType>
    SizeType MemoryListType::BufferSize() const {
        return header_->buffer_size;
    }

    template<typename T, typename SizeType>
    SizeType MemoryListType::Size() const {
        return header_->elements;
    }

    template<typename T, typename SizeType>
    SizeType MemoryListType::Capacity() const {
        return capacity_;
    }

    template<typename T, typename SizeType>
    bool MemoryListType::Empty() const {
        return header_->elements == capacity_;
    }

    template<typename T, typename SizeType>
    void MemoryListType::InitHeader(char* start, SizeType size) {
        assert (size >= kMinBufferSize);
        header_ = reinterpret_cast<Header*>(start);
        header_->magic = kMagic;
        header_->buffer_size = size;
        header_->elements = 0;
        header_->node_size = kNodeSize;
        //left used, unused, padding uninitialized
    }

    template<typename T, typename SizeType>
    bool MemoryListType::RestoreHeader(char* start, SizeType size, SizeType* capacity) {
        assert (size >= kMinBufferSize);
        header_ = reinterpret_cast<Header*>(start);
        if (header_->magic != kMagic 
                || header_->node_size != kNodeSize
                || header_->buffer_size != size) {
            return false;
        }
        auto expected_capacity = (header_->buffer_size - kMinBufferSize) / kNodeSize;
        auto padding = (reinterpret_cast<uintptr_t>(end_) - 
                reinterpret_cast<uintptr_t>(data_start_)) % kNodeSize;

        if (header_->padding != padding) {
            return false;
        }

        if (header_->used >= expected_capacity
                || header_->unused >= expected_capacity
                || header_->elements > expected_capacity) {
            return false;
        }
        *capacity = expected_capacity;
        return true;
    }

    template<typename T, typename SizeType>
    void MemoryListType::Populate() {
        uintptr_t end = reinterpret_cast<uintptr_t>(end_);
        uintptr_t start = reinterpret_cast<uintptr_t>(data_start_);
        //maximize capacity_
        capacity_ = std::numeric_limits<SizeType>::max();
        auto padding = (reinterpret_cast<uintptr_t>(end_) - 
                reinterpret_cast<uintptr_t>(data_start_)) % kNodeSize;
        uintptr_t cur = end - padding;
        assert ((cur - start) % kNodeSize == 0);
        const SizeType capacity = (cur - start) / kNodeSize;
        SizeType node_id = capacity - 1;

        header_->used = header_->unused = kInvalidNodeId;
        while (cur != start) {
            AddNodeToUnused(node_id);
            --node_id;
            cur -= kNodeSize;
        }
        assert (node_id == -1);
        header_->padding = padding;
        capacity_ = capacity;
    }

    template<typename T, typename SizeType>
    void MemoryListType::AddNodeToUsed(NodeId node_id) {
        PrependNode(node_id, header_->used);
        header_->used = node_id;
    }

    template<typename T, typename SizeType>
    void MemoryListType::AddNodeToUnused(NodeId node_id) {
#ifndef NDEBUG
        auto node = GetNodeById(node_id);
        ::memset(&node->val, 0x0, sizeof(node->val));
#endif
        PrependNode(node_id, header_->unused);
        header_->unused = node_id;
    }
    
    template<typename T, typename SizeType>
    void MemoryListType::PrependNode(NodeId node_id, NodeId header) {
        assert (header == header_->used || header == header_->unused);
        auto old_header_node = GetNodeById(header);
        if (old_header_node) {
            old_header_node->prev = node_id;
        }

        auto node = GetNodeById(node_id);
        assert (node);
        node->next = header;
        node->prev = kInvalidNodeId;
    }

    template<typename T, typename SizeType>
    typename MemoryListType::NodeId MemoryListType::UnlinkNode(NodeId node_id) {
        auto node = GetNodeById(node_id);
        assert (node);
        auto prev = GetNodeById(node->prev);
        if (prev) {
            prev->next = node->next;
        }

        auto next = GetNodeById(node->next);
        if (next) {
            next->prev = node->prev;
        }

        return node->next;
    }

    template<typename T, typename SizeType>
    typename MemoryListType::Node* MemoryListType::GetNodeById(NodeId node_id) {
        assert (node_id >= kInvalidNodeId);
        assert (node_id <= capacity_);
        if (node_id == kInvalidNodeId) {
            return nullptr;
        }
        auto addr = reinterpret_cast<uintptr_t>(data_start_) + kNodeSize * node_id;
        return reinterpret_cast<Node*>(addr);
    }

    template<typename T, typename SizeType>
    const typename MemoryListType::Node* MemoryListType::GetNodeById(NodeId node_id) const {
        assert (node_id >= kInvalidNodeId);
        assert (node_id <= capacity_);
        if (node_id == kInvalidNodeId) {
            return nullptr;
        }
        auto addr = reinterpret_cast<uintptr_t>(data_start_) + kNodeSize * node_id;
        return reinterpret_cast<const Node*>(addr);
    }

#undef MemoryListType
}

#endif   /* ----- #ifndef __MEMORY_LIST_H__  ----- */
