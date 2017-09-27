#ifndef UEP_LOG_HPP
#define UEP_LOG_HPP

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/expressions/formatters/named_scope.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

namespace uep {
namespace log {

/** Severity levels used to filter log messages. */
enum severity_level {
  trace,
  debug,
  info,
  warning,
  error
};

/** Channels used to separate performance counters from generic log
 *  messages.
 */
enum channel_type {
  basic,
  performance
};

/** Default severity-channel logger. */
typedef boost::log::sources::severity_channel_logger<
  severity_level,
  channel_type> default_logger;

/** Setup the logging library. Write the basic messages to the console
 *  and the performance messages to the given file. Can be called only
 *  once.
 */
void init(const std::string &perflog);

/** Setup the logging library. Write the basic messages to the console
 *  and the performance messages to `/dev/null`. Can be called only
 *  once.
 */
void init();

/** Setup a pair of global loggers. */
BOOST_LOG_GLOBAL_LOGGER(basic_lg, default_logger);
BOOST_LOG_GLOBAL_LOGGER(perf_lg, default_logger);

/** Write the severity level to an ostream. */
std::ostream &operator<<(std::ostream &strm, severity_level level);

/** Write the channel type to an ostream. */
std::ostream &operator<<(std::ostream &strm, channel_type level);

}}

#endif
