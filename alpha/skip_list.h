/*
 * ==============================================================================
 *
 *       Filename:  skip_list.h
 *        Created:  04/26/15 20:34:28
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __SKIP_LIST_H__
#define  __SKIP_LIST_H__

#include <cstdlib>
#include <random>
#include <iterator>
#include <iostream>
#include <type_traits>
#include "compiler.h"
#include "memory_list.h"

namespace alpha {
    template<typename Key, typename Value, int32_t kMaxLevel = 20,
        typename Comparator = std::less<Key>, typename Enable = void>
    class SkipList;

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    class SkipList<Key, Value, kMaxLevel, Comparator,
        typename std::enable_if<std::is_pod<Key>::value && std::is_pod<Value>::value
              && !std::is_pointer<Key>::value && !std::is_pointer<Value>::value>::type> {
        public:
            using key_type = Key;
            using mapped_type = Value;
            using key_compare = Comparator;
            using size_type = int32_t;
            using UniquePtr = std::unique_ptr<SkipList>;
            struct value_type {
                key_type first;
                mapped_type second;
            };

        private:
            using NodeId = size_type;
            struct Header {
                NodeId head;
                NodeId tail;
                size_type max_level;
                size_type elements;
                int64_t magic;
            };

            static const size_type kHeaderSize = sizeof(Header);
            static const int64_t kMagic = 0x631e48e40b310071;
            using LevelArray = NodeId[kMaxLevel];

            struct Node {
                NodeId prev;
                size_type level;
                LevelArray levels;
                value_type val;

                NodeId next() const {
                    return levels[0];
                }
            };

            template<typename DerivedType, typename ContainerType, 
                typename Pointer, typename Reference>
            class IteratorBase {
                public:
                    DerivedType& operator++();
                    const DerivedType operator++(int);
                    DerivedType& operator--();
                    const DerivedType operator--(int);
                    Reference operator* () const;
                    Pointer operator-> () const;
                    template<typename DerivedType2, typename ContainerType2, 
                        typename Pointer2, typename Reference2>
                    bool operator == (const IteratorBase<DerivedType2, 
                            ContainerType2, Pointer2, Reference2> & rhs) const;
                    template<typename DerivedType2, typename ContainerType2, 
                        typename Pointer2, typename Reference2>
                    bool operator != (const IteratorBase<DerivedType2, 
                            ContainerType2, Pointer2, Reference2> & rhs) const;
                protected:
                    template<typename DerivedType2, typename ContainerType2, 
                        typename Pointer2, typename Reference2>
                    friend class IteratorBase;
                    IteratorBase(ContainerType* c, NodeId node_id);
                    ContainerType* c_;
                    NodeId node_id_;
            };
            template<typename DerivedType, typename ContainerType, 
                typename Pointer, typename Reference>
            friend class IteratorBase;

            class ConstIterator : public std::iterator<
                                      std::bidirectional_iterator_tag, value_type>
                                , public IteratorBase<ConstIterator, const SkipList, 
                                                    const value_type*, const value_type&> {
                public:
                    ConstIterator();
                private:
                    ConstIterator(const SkipList* l, NodeId node_id);
                    friend class SkipList;
                    friend class Iterator;
            };

            class Iterator : public std::iterator<
                                     std::bidirectional_iterator_tag, value_type>
                                , public IteratorBase<Iterator, SkipList, 
                                                    value_type*, value_type&> {
                public:
                    Iterator();
                    Iterator(const ConstIterator&);
                private:
                    Iterator(SkipList* l, NodeId node_id);
                    friend class SkipList;
            };

        public:
            using iterator = Iterator;
            using const_iterator = ConstIterator;

            DISABLE_COPY_ASSIGNMENT(SkipList);
            static UniquePtr Create(char* start, size_type size);
            static UniquePtr Restore(char* start, size_type size);
            std::pair<iterator,bool> insert (const std::pair<key_type, mapped_type>& p);
            mapped_type& operator[] (const key_type& k);
            void erase (iterator position);
            size_type erase (const key_type& k);
            void erase (iterator first, iterator last);
            void clear();
            iterator find (const key_type& k);
            const_iterator find (const key_type& k) const;
            iterator begin();
            const_iterator begin() const;
            iterator end();
            const_iterator end() const;
            bool empty() const;
            size_type size() const;
            size_type max_size() const;
            void Dump() const;

        private:
            SkipList();
            void InitHeader();
            bool RestoreHeader(char* start, size_type size);
            NodeId PrevNode(NodeId node_id) const;
            NodeId NextNode(NodeId node_id) const;
            value_type& NodeValue(NodeId node_id) const;
            NodeId FindNode(const key_type& key, NodeId* path) const;
            void EraseNode(NodeId node_id, NodeId* path);
            bool NotGoBefore(const key_type& key, NodeId node_id, bool* equal) const;
            size_type RandomLevel();

            Header* header_;

        private:
            using MemoryListType = MemoryList<Node, size_type>;

            std::random_device rd_;
            std::mt19937 mt_;
            std::uniform_int_distribution<int> dist_;
            key_compare comparator_;
            std::unique_ptr<MemoryListType> nodes_;

            static_assert (std::is_same<NodeId, typename MemoryListType::NodeId>::value,
                    "type NodeId must be same as MemoryList<Node, size_type>::NodeId");
    };

#define SkipListType                                                                      \
    SkipList<Key, Value, kMaxLevel, Comparator,                                           \
        typename std::enable_if<std::is_pod<Key>::value && std::is_pod<Value>::value      \
              && !std::is_pointer<Key>::value && !std::is_pointer<Value>::value>::type>   \

#define SizeType typename SkipListType::size_type
#define IteratorType typename SkipListType::iterator
#define ConstIteratorType typename SkipListType::const_iterator
#define MappedType typename SkipListType::mapped_type
#define NodeIdType typename SkipListType::NodeId

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    typename SkipListType::UniquePtr SkipListType::Create(char* start, SizeType size) {
        if (size < kHeaderSize) {
            return nullptr;
        }

        std::unique_ptr<MemoryListType> nodes(
                MemoryListType::Create(start + kHeaderSize, size - kHeaderSize));
        if (nodes == nullptr) {
            return nullptr;
        }

        std::unique_ptr<SkipList> l(new SkipList);
        l->nodes_ = std::move(nodes);
        l->header_ = reinterpret_cast<Header*>(start);
        l->InitHeader();
        return std::move(l);
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    typename SkipListType::UniquePtr SkipListType::Restore(char* start, SizeType size) {
        if (size < kHeaderSize) {
            return nullptr;
        }

        std::unique_ptr<MemoryListType> nodes(
                MemoryListType::Restore(start + kHeaderSize, size - kHeaderSize));
        if (nodes == nullptr) {
            return nullptr;
        }
        std::unique_ptr<SkipList> l(new SkipList);
        l->nodes_ = std::move(nodes);
        if (l->RestoreHeader(start, size) == false) {
            return nullptr;
        }
        return std::move(l);
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    SkipListType::SkipList()
        :mt_(rd_()) {
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    void SkipListType::InitHeader() {
        header_->magic = kMagic;
        header_->max_level = kMaxLevel;
        header_->elements = 0;

        auto head = nodes_->Allocate();
        auto tail = nodes_->Allocate();
        auto node = nodes_->Get(head);
        node->prev = MemoryListType::kInvalidNodeId;
        node->level = kMaxLevel;
        for (auto level = 0; level < kMaxLevel; ++level) {
            node->levels[level] = tail;
        }

        node = nodes_->Get(tail);
        node->prev = head;
        node->level = kMaxLevel;
        for (auto level = 0; level < kMaxLevel; ++level) {
            node->levels[level] = MemoryListType::kInvalidNodeId;
        }

        header_->head = head;
        header_->tail = tail;
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    bool SkipListType::RestoreHeader(char* start, SizeType size) {
        assert (size >= kHeaderSize);
        header_ = reinterpret_cast<Header*>(start);
        if (header_->magic != kMagic
                || header_->max_level != kMaxLevel
                || header_->head != 0
                || header_->tail != 1
                || header_->elements > nodes_->Capacity() - 2) {
            return false;
        }
        return true;
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    std::pair<IteratorType, bool> SkipListType::insert (
                const std::pair<key_type, mapped_type>& p) {
        LevelArray path;
        auto node_id = FindNode(p.first, path);
        bool exists;
        if (node_id != header_->tail) {
            exists = true;
        } else {
            exists = false;
            node_id = nodes_->Allocate();
            auto node = nodes_->Get(node_id);
            node->val.first = p.first;
            node->val.second = p.second;
            node->level = RandomLevel();
            node->prev = path[0];
            for (auto level = 0; level < node->level; ++level) {
                auto prev_node = nodes_->Get(path[level]);
                node->levels[level] = prev_node->levels[level];
                prev_node->levels[level] = node_id;
            }

            auto next_node = nodes_->Get(node->next());
            next_node->prev = node_id;

            ++header_->elements;
        }
        return std::make_pair(iterator(this, node_id), !exists);
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    MappedType& SkipListType::operator[] (const key_type& k) {
        return insert(std::make_pair(k, mapped_type())).first->second;
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    void SkipListType::erase (iterator position) {
        assert (position != end());
        erase(position->first);
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    SizeType SkipListType::erase (const key_type& k) {
        LevelArray path;
        auto node_id = FindNode(k, path);
        if (node_id == header_->tail) {
            return 0;
        } else {
            EraseNode(node_id, path);
            return 1;
        }
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    void SkipListType::erase (iterator first, iterator last) {
        auto it = first;
        while (it != last) {
            auto next = it;
            ++next;
            erase(it);
            it = next;
        }
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    void SkipListType::clear() {
        nodes_->Clear();
        InitHeader();
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    IteratorType SkipListType::find (const key_type& k) {
        LevelArray path;
        return iterator(this, FindNode(k, path));
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    ConstIteratorType SkipListType::find (const key_type& k) const {
        LevelArray path;
        return const_iterator(this, reinterpret_cast<SkipList*>(this)->FindNode(k, &path));
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    IteratorType SkipListType::begin () {
        return iterator(this, nodes_->Get(header_->head)->next());
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    ConstIteratorType SkipListType::begin () const {
        return const_iterator(this, nodes_->Get(header_->head)->next());
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    IteratorType SkipListType::end () {
        return iterator(this, header_->tail);
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    ConstIteratorType SkipListType::end () const {
        return const_iterator(this, header_->tail);
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    bool SkipListType::empty() const {
        return header_->elements == 0;
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    SizeType SkipListType::size() const {
        return header_->elements;
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    SizeType SkipListType::max_size() const {
        return nodes_->Capacity() - 2;
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    void SkipListType::Dump() const {
        std::cout << "************************************************************\n";
        for (auto node_id = header_->head; node_id != MemoryListType::kInvalidNodeId; ) {
            auto node = nodes_->Get(node_id);
            printf("id = %10d, prev = %10d, next = %10d, level = %10d, levels = [", 
                    node_id, node->prev, node->next(), node->level);
            for (auto level = 0; level < node->level;) {
                std::cout << node->levels[level];
                if (++level < node->level) {
                    std::cout << ", ";
                }
            }
            printf("], key = %10d\n", node->val.first);
            node_id = node->next();
        }
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    NodeIdType SkipListType::PrevNode(NodeId node_id) const {
        assert (node_id != MemoryListType::kInvalidNodeId);
        if (node_id == header_->head) {
            return header_->head;
        } else {
            auto node = nodes_->Get(node_id);
            return node->prev;
        }
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    NodeIdType SkipListType::NextNode(NodeId node_id) const {
        assert (node_id != MemoryListType::kInvalidNodeId);
        if (node_id == header_->tail) {
            return header_->tail;
        } else {
            auto node = nodes_->Get(node_id);
            return node->next();
        }
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    typename SkipListType::value_type& SkipListType::NodeValue(NodeId node_id) const {
        assert (node_id != MemoryListType::kInvalidNodeId);
        assert (node_id != header_->head);
        assert (node_id != header_->tail);

        return nodes_->Get(node_id)->val;
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    NodeIdType SkipListType::FindNode(const key_type& key, NodeId* path) const {
        assert (path);
        static_assert(kMaxLevel > 1, "KMaxLevel must be large than 1");
        int current_level = kMaxLevel - 1;
        auto current_node_id = header_->head;
        auto current_node = nodes_->Get(current_node_id);
        auto prev_node_id = current_node_id;
        NodeId target = header_->tail;
        while (current_level >= 0) {
            bool equal;
            assert (current_node->level >= current_level);
            while (current_level >= 0 
                    && NotGoBefore(key, current_node->levels[current_level], &equal)) {
                if (equal) {
                    prev_node_id = current_node_id;
                    target = current_node->levels[current_level];
                    break;
                } else {
                    prev_node_id = current_node->levels[current_level];
                }
                current_node_id = current_node->levels[current_level];
                current_node = nodes_->Get(current_node_id);
            }
            path[current_level] = prev_node_id;
            --current_level;
        }
        return target;
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    void SkipListType::EraseNode(NodeId node_id, NodeId* path) {
        assert (path);
        assert (node_id != header_->head);
        assert (node_id != header_->tail);
        assert (node_id != MemoryListType::kInvalidNodeId);

        auto node = nodes_->Get(node_id);
        auto next_node = nodes_->Get(node->next());
        next_node->prev = node->prev;

        for (auto level = 0; level < node->level; ++level) {
            auto level_node = nodes_->Get(path[level]);
            assert (level_node->levels[level] == node_id);
            level_node->levels[level] = node->levels[level];
        }

        --header_->elements;
        nodes_->Deallocate(node_id);
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    bool SkipListType::NotGoBefore(const key_type& key, NodeId node_id, bool* equal) const {
        *equal = false;
        if (node_id == header_->tail) {
            return false;
        } else if (node_id == header_->head) {
            return true;
        } else {
            auto node = nodes_->Get(node_id);
            assert (node);
            bool go_before = comparator_(key, node->val.first);
            bool go_after = comparator_(node->val.first, key);
            *equal = !go_before && !go_after;
            return !go_before;
        }
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    SizeType SkipListType::RandomLevel() {
        size_t level = 1;
        while ((dist_(mt_) & 0xFFFF) < 0.25 * 0xFFFF) {
            ++level;
        }
        return level < kMaxLevel ? level : kMaxLevel;
    }

#define IteratorBaseType IteratorBase<DerivedType, ContainerType, Pointer, Reference>
    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    template<typename DerivedType, typename ContainerType, 
        typename Pointer, typename Reference>
    SkipListType::IteratorBaseType::IteratorBase(ContainerType* c, NodeId node_id)
        :c_(c), node_id_(node_id) {
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    template<typename DerivedType, typename ContainerType, 
        typename Pointer, typename Reference>
    DerivedType& SkipListType::IteratorBaseType::operator++() {
        node_id_ = c_->NextNode(node_id_);
        return static_cast<DerivedType&>(*this);
    }
    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    template<typename DerivedType, typename ContainerType, 
        typename Pointer, typename Reference>
    const DerivedType SkipListType::IteratorBaseType::operator++(int) {
        auto res = static_cast<DerivedType&>(*this);
        ++*this;
        return res;
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    template<typename DerivedType, typename ContainerType, 
        typename Pointer, typename Reference>
    DerivedType& SkipListType::IteratorBaseType::operator--() {
        node_id_ = c_->PrevNode(node_id_);
        return static_cast<DerivedType&>(*this);
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    template<typename DerivedType, typename ContainerType, 
        typename Pointer, typename Reference>
    const DerivedType SkipListType::IteratorBaseType::operator--(int) {
        auto res = static_cast<DerivedType&>(*this);
        --*this;
        return res;
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    template<typename DerivedType, typename ContainerType, 
        typename Pointer, typename Reference>
    Reference SkipListType::IteratorBaseType::operator* () const {
        return c_->NodeValue(node_id_);
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    template<typename DerivedType, typename ContainerType, 
        typename Pointer, typename Reference>
    Pointer SkipListType::IteratorBaseType::operator-> () const {
        return &**this;
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    template<typename DerivedType, typename ContainerType, 
        typename Pointer, typename Reference>
    template<typename DerivedType2, typename ContainerType2, 
        typename Pointer2, typename Reference2>
    bool SkipListType::IteratorBaseType::operator == (
            const IteratorBase<DerivedType2, ContainerType2, Pointer2, Reference2> & rhs) 
        const {
        return c_ == rhs.c_ && node_id_ == rhs.node_id_;
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    template<typename DerivedType, typename ContainerType, 
        typename Pointer, typename Reference>
    template<typename DerivedType2, typename ContainerType2, 
    typename Pointer2, typename Reference2>
    bool SkipListType::IteratorBaseType::operator != (
            const IteratorBase<DerivedType2, ContainerType2, Pointer2, Reference2> & rhs) 
        const {
        return !(*this == rhs);
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    SkipListType::ConstIterator::ConstIterator()
        :IteratorBase<ConstIterator, const SkipList, 
        const value_type*, const value_type&>(nullptr, MemoryListType::kInvalidNodeId){
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    SkipListType::ConstIterator::ConstIterator(const SkipList* l, NodeId node_id)
        :IteratorBase<ConstIterator, const SkipList, 
        const value_type*, const value_type&>(l, node_id){
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    SkipListType::Iterator::Iterator()
        :IteratorBase<Iterator, SkipList, 
        value_type*, value_type&>(nullptr, MemoryListType::kInvalidNodeId){
    }

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    SkipListType::Iterator::Iterator(const ConstIterator& it)
        :IteratorBase<Iterator, SkipList, 
        value_type*, value_type&>(it.c_, it.node_id_){
    }
    

    template<typename Key, typename Value, int32_t kMaxLevel, typename Comparator>
    SkipListType::Iterator::Iterator(SkipList* l, NodeId node_id)
        :IteratorBase<Iterator, SkipList, 
        value_type*, value_type&>(l, node_id){
    }
                    

#undef SkipListType
#undef SizeType
#undef IteratorType
#undef ConstIteratorType
#undef MappedType
#undef NodeIdType
#undef IteratorBaseType
}

#endif   /* ----- #ifndef __SKIP_LIST_H__  ----- */
