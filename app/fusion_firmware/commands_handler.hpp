#include "board_states.hpp"
#include "buffers.hpp"
#include "fusion_bsp/board.hpp"
#include "lattice_fpga.hpp"
#include <modm/processing.hpp>
#include <modm/processing/rtos.hpp>

using namespace Board;
using namespace modm::platform;
using namespace std::chrono_literals;

enum class Commands : uint8_t
{
    LoadFPGABitstream = 1,
    SetStatusLed = 2,
    SetFPGAReset = 3,
    WriteFlash = 4,
    Unknown = 255
};

class CommandsHandler : modm::rtos::Thread
{
    constexpr static inline uint16_t buffer_size = 4 * 1024;


    static inline uint8_t* buffer()
    {
        static uint8_t _buffer[buffer_size];
        return _buffer;
    }

    static inline void _ex_set_stats_led(const USBCommands::buffer_t& packet)
    {
        STATUS_LED::set(packet.data[1]);
    }

    static inline void _ex_set_fpga_reset(const USBCommands::buffer_t& packet)
    {
        FPGA_RESET::set(packet.data[1]);
    }

    static inline void _ex_load_bitstream(const USBCommands::buffer_t& packet)
    {
        board_state = BoardState::ProgrammingFPGA;
        uint32_t bitstream_size = (packet.data[2] << 16) + (packet.data[3] << 8) + packet.data[4];
        uint8_t bitstream_source = packet.data[1];
        LED1::set(0);
        FPGA_RESET::set(0);
        if (bitstream_source == 0)
        {
            auto loader = FPGA::begin_flash(bitstream_size);
            while (!loader.complete())
            {
                LED1::set(FPGA_CFG::INIT::read());
                auto bs = USBDataQueue::pull_rx();
                loader << std::pair { bs.data, bs.count };
            }
        }
        else if (bitstream_source == 1)
        {
            auto loader = FPGA::begin_flash(Flash::partition_size);
            // in this case bitstream_size is just the partition index
            uint32_t address = bitstream_size * Flash::partition_size;
            while (!loader.complete())
            {
                RF_CALL_BLOCKING(Flash::memory.read(buffer(), address, buffer_size));
                loader << std::pair { reinterpret_cast<char*>(buffer),
                    static_cast<uint16_t>(buffer_size) };
                address += buffer_size;
            }
        }
        FPGA::end_flash();
        vTaskDelay(1);
        FPGA_RESET::set(1);
        LED1::set(FPGA_CFG::DONE::read());
        board_state = BoardState::Running;
    }

    static inline void _ex_write_flash(const USBCommands::buffer_t& packet)
    {
        // just split the flash into 32 1Mb blocks
        uint32_t block_index = packet.data[1];
        uint32_t written = 0;
        while (written < Flash::partition_size)
        {
            LED1::toggle();
            uint16_t buffer_level = 0;
            uint16_t buffer_capacity = buffer_size;
            while (buffer_level < buffer_size && written < Flash::partition_size)
            {
                auto bs = USBDataQueue::pull_rx();
                auto extracted = std::min(buffer_capacity, bs.count);
                memcpy(buffer() + buffer_level, bs.data, extracted);
                buffer_level += extracted;
                buffer_capacity -= extracted;
                if (extracted != bs.count)
                {
                    RF_CALL_BLOCKING(Flash::memory.write(
                        buffer(), block_index * Flash::partition_size + written, buffer_size));
                    written += buffer_size;
                    auto remaining = bs.count - extracted;
                    memcpy(buffer(), bs.data + extracted, remaining);
                    buffer_level = remaining;
                    buffer_capacity = buffer_size - buffer_level;
                }
            }
            RF_CALL_BLOCKING(Flash::memory.write(
                buffer(), block_index * Flash::partition_size + written, buffer_size));
            written += buffer_size;
        }
    }

    static inline Commands command_type(const USBCommands::buffer_t& packet)
    {
        if (packet.count > 0)
        {
            return static_cast<Commands>(packet.data[0]);
        }
        return Commands::Unknown;
    }

    inline void execute_command(const USBCommands::buffer_t& packet)
    {
        switch (command_type(packet))
        {
            case Commands::SetStatusLed:
                _ex_set_stats_led(packet);
                break;
            case Commands::SetFPGAReset:
                _ex_set_fpga_reset(packet);
                break;
            case Commands::LoadFPGABitstream:
                _ex_load_bitstream(packet);
                break;
            case Commands::WriteFlash:
                _ex_write_flash(packet);
                break;
            default:
                break;
        }
    }

public:
    CommandsHandler() : Thread(1, 2048, "Commands Handler") { }

    void run()
    {
        while (1)
        {
            auto next_command = USBCommands::pull_rx();
            execute_command(next_command);
        }
    }
};