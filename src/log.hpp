#ifndef LOG_HPP
#define LOG_HPP

#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/manipulators.hpp>
#include <boost/utility/empty_deleter.hpp>

enum severity_level
{
  debug,
  info,
  warning,
  error
};

typedef boost::log::sources::severity_logger<severity_level> default_logger;

BOOST_LOG_ATTRIBUTE_KEYWORD(stat_stream, "StatisticStream", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(stat_value, "StatisticValue", double)

class stat_backend :
  public boost::log::sinks::basic_sink_backend<boost::log::sinks::synchronized_feeding> {
private:
  typedef std::map<std::string, std::vector<double> > stat_map_t;
public:
  explicit stat_backend(boost::shared_ptr<std::ostream> &output_stream) :
    os(output_stream) {}

  ~stat_backend() {
    *os << "{";
    for (auto i = stat_map.cbegin(); i != stat_map.cend(); ++i) {
      *os << '"' << i->first << '"';
      *os << ":[";
      const std::vector<double> &vec = i->second;
      for (auto j = vec.cbegin(); j != vec.cend(); ++j) {
	*os << *j;
	if (++(std::vector<double>::const_iterator(j)) != vec.cend()) *os << ',';
      }
      *os << "]";
      if (++(stat_map_t::const_iterator(i)) != stat_map.cend()) *os << ',';
    }
    *os << "}";
    //*os.flush();
  }

  // The method is called for every log record being put into the sink backend
  void consume(const boost::log::record_view &rec) {
    // First, acquire statistic information stream name
    boost::log::value_ref<std::string, tag::stat_stream > name = rec[stat_stream];
    if (name) {
      // Next, get the statistic value change
      boost::log::value_ref< double, tag::stat_value > value = rec[stat_value];
      if (value)
	stat_map[name.get()].push_back(value.get());
    }
  }

private:
  stat_map_t stat_map;
  boost::shared_ptr<std::ostream> os;
};

inline void setup_stat_sink(const std::string &fname) {
  using namespace std;
  using boost::shared_ptr;

  shared_ptr<ostream> ofs(new ofstream(fname, ios_base::trunc));
  shared_ptr<stat_backend> backend(new stat_backend(ofs));

  typedef boost::log::sinks::synchronous_sink<stat_backend> log_sink_type;
  boost::shared_ptr<log_sink_type> log_sink(new log_sink_type(backend));
  log_sink->set_filter(boost::log::expressions::has_attr(stat_stream));
  boost::log::core::get()->add_sink(log_sink);
}

#define PUT_STAT(lg, stat_stream_name, value)\
  if (true) {\
    BOOST_LOG_SCOPED_LOGGER_TAG(lg, "StatisticStream", stat_stream_name);\
    BOOST_LOG_SEV(lg, debug) <<\
      boost::log::add_value("StatisticValue", (double)(value)); \
  } else ((void)0)

#endif
