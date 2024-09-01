#pragma once

#include <modm/processing/rtos.hpp>
#include <modm/processing.hpp>
#include <modm/processing/rtos/queue.hpp>

struct USBCommand
{
};

template <uint16_t buffer_size>
struct Buffer
{
    static inline constexpr auto capacity = buffer_size;
    char *data;
    uint16_t count = 0;
};

template <uint16_t buffer_size, uint8_t buffer_count, typename tag>
struct BufferPool
{
    using buffer_t = Buffer<buffer_size>;
    static inline constexpr auto count = buffer_count;
    BufferPool() : _free{buffer_count}
    {
        for (uint8_t i = 0; i < buffer_count; i++)
        {
            _free.append(i);
        }
    }

    inline buffer_t take_one()
    {
        uint8_t index;
        _free.get(index);
        return buffer_t{_address(index), 0};
    }

    inline std::optional<buffer_t> maybe_take_one(int timeout = 0)
    {
        uint8_t index;
        if (_free.get(index, timeout))
        {
            return buffer_t{_address(index), 0};
        }
        return std::nullopt;
    }

    inline void release_one(const buffer_t &b)
    {
        _free.append(_index_of(b));
    }

    inline char *base_address()
    {
        return _data;
    }

private:
    char _data[buffer_size * buffer_count]__attribute__((aligned(4)));
    modm::rtos::Queue<uint8_t> _free;

    inline char *_address(uint8_t index)
    {
        return _data + static_cast<uint32_t>(index) * buffer_size;
    }

    inline uint8_t _index_of(const buffer_t &buffer)
    {
        return static_cast<uint8_t>((buffer.data - _data) / buffer_size);
    }
};

template <typename BufferPool, typename tag>
struct BufferQueue
{
    using buffer_t = typename BufferPool::buffer_t;
    static inline constexpr auto buffer_capacity = buffer_t::capacity;
    static inline constexpr auto buffer_count = BufferPool::count;

    struct entry
    {
        uint16_t index;
        uint16_t count;
    };

private:
    modm::rtos::Queue<entry> _ready;
    char *_data;

    inline uint16_t _index_of(const buffer_t &buffer)
    {
        return static_cast<uint16_t>((buffer.data - _data) / buffer_t::capacity);
    }

    inline char *_address(uint16_t index)
    {
        return _data + static_cast<uint32_t>(index) * buffer_t::capacity;
    }

public:
    BufferQueue(BufferPool &pool) : _ready{buffer_count}, _data{pool.base_address()}
    {
    }

    inline buffer_t get_ready_buffer()
    {
        entry e;
        _ready.get(e);
        return {_address(e.index), e.count};
    }

    inline void push_ready_buffer(const buffer_t &b)
    {
        _ready.append({_index_of(b), b.count});
    }

    std::optional<buffer_t> maybe_ready_buffer(int timeout=0)
    {
        entry e;
        if (_ready.get(e, timeout))
            return buffer_t{_address(e.index), e.count};
        return std::nullopt;
    }
};

template <typename Queue_t, typename base_t>
struct auto_release_buffer_t : base_t
{
    auto_release_buffer_t(const base_t &b)
    {
        this->data = b.data;
        this->count = b.count;
    }

    auto_release_buffer_t(const auto_release_buffer_t &b) = delete;

    auto_release_buffer_t(base_t &&b)
    {
        this->data = b.data;
        this->count = b.count;
    }

    auto_release_buffer_t(auto_release_buffer_t &&b)
    {
        this->data = b.data;
        this->count = b.count;
        this->needs_release = b.needs_release;
        b.needs_release = false;
    }

    inline auto_release_buffer_t &operator=(auto_release_buffer_t &&other)
    {
        this->data = other.data;
        this->count = other.count;
        this->needs_release = other.needs_release;
        other.needs_release = false;
        return *this;
    }

    ~auto_release_buffer_t()
    {
        if (needs_release)
            Queue_t::release(*this);
    }

    inline void set_no_release()
    {
        needs_release = false;
    }

private:
    bool needs_release = true;
};

template <uint16_t size, uint16_t count, typename tag>
struct BidirQueue
{

private:
    static inline BufferPool<size, count, tag> _pool{};
    static inline BufferQueue<decltype(_pool), tag> _tx_queue{_pool};
    static inline BufferQueue<decltype(_pool), tag> _rx_queue{_pool};

public:
    using buffer_t = decltype(_pool)::buffer_t;
    static inline constexpr auto buffer_capacity = buffer_t::capacity;
    using auto_buffer_t = auto_release_buffer_t<BidirQueue<size, count, tag>, buffer_t>;

    static inline auto_buffer_t new_empty_packet()
    {
        return _pool.take_one();
    }

    static inline std::optional<auto_buffer_t> maybe_new_empty_packet()
    {
        auto maybe_packet = _pool.maybe_take_one();
        if (maybe_packet)
            return std::optional<auto_buffer_t>{*maybe_packet};
        else
            return std::nullopt;
    }

    static inline void push_tx(auto_buffer_t &&b)
    {
        b.set_no_release();
        _tx_queue.push_ready_buffer(b);
    }

    static inline void push_rx(auto_buffer_t &&b)
    {
        b.set_no_release();
        _rx_queue.push_ready_buffer(b);
    }

    static inline std::optional<auto_buffer_t> maybe_rx(int timeout=0)
    {

        auto maybe_packet = _rx_queue.maybe_ready_buffer( timeout);
        if (maybe_packet)
            return std::optional<auto_buffer_t>{*maybe_packet};
        else
            return std::nullopt;
    }

    static inline std::optional<auto_buffer_t> maybe_tx(int timeout=0)
    {
        auto maybe_packet = _tx_queue.maybe_ready_buffer(timeout);
        if (maybe_packet)
            return std::optional<auto_buffer_t>{*maybe_packet};
        else
            return std::nullopt;
    }

    static inline auto_buffer_t pull_rx()
    {
        return auto_buffer_t{_rx_queue.get_ready_buffer()};
    }

    static inline auto_buffer_t pull_tx()
    {
        return auto_buffer_t{_tx_queue.get_ready_buffer()};
    }

    friend auto_buffer_t;

protected:
    static inline void release(const buffer_t &buffer)
    {
        _pool.release_one(buffer);
    }
};

struct usb_tag
{
};

using USBCommands = BidirQueue<8, 8, usb_tag>;
using USBDataQueue = BidirQueue<1024*4, 8, usb_tag>;
