#include <boost/log/trivial.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include "logger.h"


namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace fs = boost::dll::fs;

void initLogging(const std::string fileName) {
    // to console
    auto consoleHandler = logging::add_console_log();
    consoleHandler->set_filter(
        logging::trivial::severity >= logging::trivial::debug
    );
    // to file
    if (fileName != "") {
        fs::path exePath = boost::dll::program_location();
        boost::filesystem::path filePath = exePath / "pedal-buttons.log";
        auto fileHandler = logging::add_file_log(
            keywords::file_name = filePath.string(),
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
    logging::add_common_attributes();
}
