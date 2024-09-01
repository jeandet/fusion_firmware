
#pragma once
#include <modm/math/units.hpp>
#include <stdint.h>
#include <tuple>

template <typename Spi>
struct lattice_fpga_flash_stream
{
    uint32_t expected_size;
    uint32_t transmitted = 0;
    lattice_fpga_flash_stream(uint32_t expected_size)
            : expected_size { expected_size }, transmitted { 0 }
    {
    }

    inline lattice_fpga_flash_stream& operator<<(std::pair<char*, uint16_t>& bitstream)
    {
        if (!complete())
        {
            Spi::transferBlocking(
                reinterpret_cast<uint8_t*>(bitstream.first), bitstream.first, bitstream.second);
            transmitted += bitstream.second;
        }

        return *this;
    }

    inline lattice_fpga_flash_stream& operator<<(std::pair<char*, uint16_t>&& bitstream)
    {
        if (!complete())
        {
            Spi::transferBlocking(reinterpret_cast<uint8_t*>(bitstream.first),
                reinterpret_cast<uint8_t*>(bitstream.first), bitstream.second);
            transmitted += bitstream.second;
        }

        return *this;
    }

    inline bool complete() const { return transmitted >= expected_size; }
};

template <typename Spi, typename Program, typename HaveData, typename Init, uint32_t addr>
struct lattice_fpga_t
{

    template <class SystemClock, modm::baudrate_t baudrate>
    static bool initialize()
    {
        using namespace modm::platform;

        Spi::template initialize<SystemClock, baudrate>();
        Spi::setDataMode(Spi::DataMode::Mode1);
        Program::setOutput(modm::Gpio::High);
        Init::setOutput(Gpio::OutputType::OpenDrain);
        return true;
    }

    // only SCM mode supported!
    static inline auto begin_flash(uint32_t expected_size)
    {
        Program::set(0);
        while (Init::read() == 1)
            ;
        Program::set(1);
        while (Init::read() == 0)
            ;
        return lattice_fpga_flash_stream<Spi> { expected_size };
    }

    static inline bool end_flash() { return true; }

    static inline bool have_data() { return HaveData::read(); }

    inline static constexpr uint32_t address = addr;

    static inline volatile uint32_t& value(uint32_t index) noexcept
    {
        return memory_space()[index];
    }

    static inline volatile uint32_t* memory_space() noexcept
    {
        return reinterpret_cast<uint32_t*>(address);
    }
};
