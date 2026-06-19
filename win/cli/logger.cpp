#include <boost/log/trivial.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/expressions.hpp>
#include "logger.h"

#include <filesystem>
#include <iostream>


namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

void initLogging(const std::string fileName) {
    auto consoleHandler = logging::add_console_log();
    consoleHandler->set_filter(
        logging::trivial::severity >= logging::trivial::debug
    );
    if (fileName != "") {
        try {
            std::filesystem::path logPath = std::filesystem::current_path() / fileName;
            auto fileHandler = logging::add_file_log(
                keywords::file_name = logPath.string(),
                keywords::rotation_size = 10 * 1024 * 1024,
                keywords::max_size = 50 * 1024 * 1024,
                keywords::format = (
                    expr::stream
                    << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S")
                    << " [" << logging::trivial::severity
                    << "] " << expr::smessage
                )
            );
            fileHandler->set_filter(
                logging::trivial::severity >= logging::trivial::info
            );
        }
        catch (const std::exception& e) {
            std::cerr << "Не удалось создать лог-файл: " << e.what() << std::endl;
        }
    }
    logging::add_common_attributes();
}
