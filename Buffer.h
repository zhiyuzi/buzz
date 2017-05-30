#pragma once

#include <string>
#include <vector>

#include <assert.h>
#include <stddef.h>

namespace buzz
{
    //
    // +-----------------------------------------------------+
    // | prependable bytes | readable bytes | writable bytes |
    // |                   |   (CONTENT)    |                |
    // +-------------------+----------------+----------------+
    // |                   |                |                |
    // 0        <=     readerIndex  <=  writerIndex   <=   size   
    //
    class Buffer
    {
    public:
        static const size_t kCheapPrepend = 8;
        static const size_t kInitialSize = 1024;

        Buffer()
            : m_buffer(kCheapPrepend + kInitialSize),
            m_reader_index(kCheapPrepend),
            m_writer_index(kCheapPrepend)
        {
            assert(ReadableBytes() == 0);
            assert(WritableBytes() == kInitialSize);
            assert(PrependableBytes() == kCheapPrepend);
        }

        void swap(Buffer& rhs)
        {
            m_buffer.swap(rhs.m_buffer);
            std::swap(m_reader_index, rhs.m_reader_index);
            std::swap(m_writer_index, rhs.m_writer_index);
        }

        size_t ReadableBytes() const { return m_writer_index - m_reader_index; }
        size_t WritableBytes() const { return m_buffer.size() - m_writer_index;}
        size_t PrependableBytes() const { return m_reader_index; }

        const char* Peek() const { return Begin() + m_reader_index; }

        void Retrieve(size_t len)
        {
            assert(len <= ReadableBytes());
            m_reader_index += len;
        }

        void RetrieveUntil(const char* end)
        {
            assert(Peek() <= end && BeginWrite() >= end);
            Retrieve(end - Peek());
        }

        void RetrieveAll()
        {
            m_reader_index = m_writer_index = kCheapPrepend;
        }

        std::string RetrieveAsString()
        {
            std::string str(Peek() , ReadableBytes());

            RetrieveAll();
            
            return str;
        }

        void Append(const char* data, size_t len)
        {
            EnsureWritableBytes(len);
            std::copy(data, data + len, BeginWrite());
            HasWriten(len);
        }

        void Append(const void* data, size_t len) { Append(static_cast<const char*>(data), len); }

        void Append(const std::string& str) { Append(str.data() , str.size()); }

        void HasWriten(size_t len) { m_writer_index += len; }

        char* BeginWrite() { return Begin() + m_writer_index; }
        const char* BeginWrite() const { return Begin() + m_writer_index; }

        void Prepend(const void* data, size_t len)
        {
            assert(len <= PrependableBytes());
            m_reader_index -= len;
            const char* t = static_cast<const char*>(data);
            std::copy(t, t + len, Begin() + len);
        }

        void Shrink(size_t reserve)
        {
            Buffer other;
            other.EnsureWritableBytes(ReadableBytes() + reserve);
            other.Append(Begin(), ReadableBytes());
            swap(other);
        }

        void EnsureWritableBytes(size_t len)
        {
            if (WritableBytes() < len) {
                MakeSpace(len);
            }
            assert(WritableBytes() >= len);
        }

        ssize_t ReadFd(int fd, int* err_code);
    private:
        std::vector<char> m_buffer;

        size_t m_reader_index;
        size_t m_writer_index;

        char* Begin() { return m_buffer.data(); }
        const char* Begin() const { return m_buffer.data(); }

        void MakeSpace(size_t len)
        {
            if (WritableBytes() + PrependableBytes() < len + kCheapPrepend) {
               
                m_buffer.resize(m_writer_index + len);
            } else {
                assert(kCheapPrepend < m_reader_index);

                size_t readable = ReadableBytes();
                std::copy(Begin() + m_reader_index , Begin() + m_writer_index,
                          Begin() + kCheapPrepend);

                m_reader_index = kCheapPrepend;
                m_writer_index = m_reader_index + readable;

                assert(readable == ReadableBytes());
            }
        }
    };
}