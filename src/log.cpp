#include "log.hpp"

namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace src = boost::log::sources;

namespace uep {
namespace log {

BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(basic_lg,
				  default_logger,
				  (boost::log::keywords::channel =
				   basic));
BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(perf_lg,
				  default_logger,
				  (boost::log::keywords::channel =
				   performance));

static bool was_inited = false;

void init() {
  init("/dev/null");
}

void init(const std::string &perflog) {
  init(perflog, trace);
}

void init(const std::string &perflog, severity_level console_level) {
  if (was_inited) throw std::runtime_error("uep::log::init was called twice");
  else was_inited = true;

  logging::add_common_attributes();

  auto filter_perf =  expr::attr<channel_type>("Channel") == performance;
  auto filter_basic =  expr::attr<channel_type>("Channel") == basic &&
    expr::attr<severity_level>("Severity") >= console_level;

  auto formatter_perf = expr::stream
    << expr::format_date_time<boost::posix_time::ptime>("TimeStamp",
							"[%Y-%m-%d %H:%M:%S.%f] ")
    // << expr::format_named_scope("Scopes", "(%n) ")
    << "| " << expr::message;

  auto formatter_basic = expr::stream
    << expr::format_date_time<boost::posix_time::ptime>("TimeStamp",
							"[%Y-%m-%d %H:%M:%S.%f] ")
    // << expr::format_named_scope("Scopes", "(%n) ")
    << expr::attr<severity_level>("Severity") << " "
    << "| " << expr::message;

  auto file_sink = logging::add_file_log(perflog);
  file_sink->set_filter(filter_perf);
  file_sink->locked_backend()->auto_flush(true);
  file_sink->set_formatter(formatter_perf);

  auto console_sink = logging::add_console_log();
  console_sink->set_filter(filter_basic);
  console_sink->locked_backend()->auto_flush(true);
  console_sink->set_formatter(formatter_basic);
}

std::ostream &operator<<(std::ostream &strm, severity_level level) {
  static const char *strings[] =
    {
      "trace",
      "debug",
      "info",
      "warning",
      "error"
    };

  if (static_cast<std::size_t>(level) < sizeof(strings) / sizeof(*strings))
    strm << strings[level];
  else
    strm << static_cast<int>(level);

  return strm;
}

std::ostream &operator<<(std::ostream &strm, channel_type level) {
  static const char *strings[] =
    {
      "basic",
      "performance"
    };

  if (static_cast<std::size_t>(level) < sizeof(strings) / sizeof(*strings))
    strm << strings[level];
  else
    strm << static_cast<int>(level);

  return strm;
}

}}
