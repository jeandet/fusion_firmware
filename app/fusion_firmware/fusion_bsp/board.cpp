#include "board.hpp"


Board::LoggerDevice loggerDevice;

// Set all four logger streams to use the UART
modm::log::Logger modm::log::debug(loggerDevice);
modm::log::Logger modm::log::info(loggerDevice);
modm::log::Logger modm::log::warning(loggerDevice);
modm::log::Logger modm::log::error(loggerDevice);

// Default all calls to printf to the UART
modm_extern_c void
putchar_(char c)
{
	loggerDevice.write(c);
}

Board::Flash::memory_t Board::Flash::memory = Board::Flash::memory_t{};