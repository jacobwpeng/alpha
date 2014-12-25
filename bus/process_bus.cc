/*
 * ==============================================================================
 *
 *       Filename:  process_bus.cc
 *        Created:  12/21/14 14:05:03
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include "process_bus.h"
#include <cassert>

namespace alpha {
    ProcessBus::ProcessBus()
        : header_ (nullptr) {
    }

    std::unique_ptr<ProcessBus> ProcessBus::RestoreFrom (
        const alpha::Slice& filepath, size_t size) {
        if (size < kHeaderSize) {
            return nullptr;
        }

        std::unique_ptr<ProcessBus> bus (new ProcessBus);
        bus->file_.reset (new MMapFile (filepath, size));
        if (bus->file_->Inited() == false) {
            return nullptr;
        }

        ProcessBus::Header* header = reinterpret_cast<ProcessBus::Header*> (
                                         bus->file_->start());
        if (header->magic_number != ProcessBus::Header::kMagicNumber) {
            return nullptr;
        }

        bus->header_ = header;
        void* start = static_cast<char*> (bus->file_->start()) + kHeaderSize;
        assert (bus->file_->size() > kHeaderSize);
        bus->buf_ = std::move (alpha::RingBuffer::RestoreFrom (
                    start, bus->file_->size() - kHeaderSize));
        bus->filepath_ = filepath.ToString();
        return bus;
    }

    std::unique_ptr<ProcessBus> ProcessBus::CreateFrom (
            const alpha::Slice& filepath, size_t size) {

        if (size < kHeaderSize) {
            return nullptr;
        }

        std::unique_ptr<ProcessBus> bus (new ProcessBus);
        bus->file_.reset (new MMapFile (filepath, size,
                          MMapFile::create_if_not_exists 
                              | MMapFile::truncate
                              | MMapFile::zero_clear ));

        if (bus->file_->Inited() == false) {
            return nullptr;
        }

        ProcessBus::Header* header = reinterpret_cast<ProcessBus::Header*>
                                     (bus->file_->start());
        header->magic_number = ProcessBus::Header::kMagicNumber;
        bus->header_ = header;
        void* start = static_cast<char*> (bus->file_->start()) + kHeaderSize;
        assert (bus->file_->size() > kHeaderSize);
        bus->buf_ = std::move (RingBuffer::CreateFrom (start, 
                    bus->file_->size() - kHeaderSize));
        bus->filepath_ = filepath.ToString();
        return bus;
    }

    std::unique_ptr<ProcessBus> ProcessBus::RestoreOrCreate(
            const alpha::Slice& filepath, size_t size, bool force) {
        auto bus = RestoreFrom(filepath, size);
        if (!bus && force) {
            bus = CreateFrom(filepath, size);
        }
        return bus;
    }

    bool ProcessBus::Write (const char* buf, int len) {
        return buf_->Push (buf, len);
    }

    char* ProcessBus::Read (int* plen) {
        return buf_->Pop (plen);
    }

    bool ProcessBus::empty() const {
        return buf_->empty();
    }

    std::string ProcessBus::filepath() const {
        return filepath_;
    }
}
