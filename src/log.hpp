#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/manipulators.hpp>
#include <boost/utility/empty_deleter.hpp>

enum severity_level {
  debug,
  info,
  warning,
  error
};

enum statistic_type {
  scalar,
  counter
};

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(stat_stream, "StatisticStream", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(stat_type, "StatisticType", statistic_type)
BOOST_LOG_ATTRIBUTE_KEYWORD(stat_value, "StatisticValue", double)

class stat_backend;
typedef boost::log::sources::severity_logger<severity_level> default_logger;
typedef boost::log::sinks::synchronous_sink<stat_backend> default_stat_sink;
typedef boost::log::sinks::synchronous_sink<
  boost::log::sinks::text_ostream_backend> default_cerr_sink;

class stat_scalar_backend :
  public boost::log::sinks::basic_sink_backend<
  boost::log::sinks::synchronized_feeding> {
public:
  void consume(const boost::log::record_view &rec) {
    boost::log::value_ref<std::string, tag::stat_stream > name = rec[stat_stream];
    if (!name) return;
    boost::log::value_ref<statistic_type, tag::stat_type > type = rec[stat_type];
    if (!type || !type.get() == scalar) return;
    boost::log::value_ref<double, tag::stat_value> value = rec[stat_value];
    if (!value) return;
    auto i = stat_map.find(name.get());
    if (i == stat_map.end())
      stat_map.insert(make_pair(name.get(), std::vector<double>(1, value.get())));
    else
      i->second.push_back(value.get());
  }

  void print_to_json(std::ostream &ostr) {
    ostr << "{";
    for (auto i = stat_map.cbegin(); i != stat_map.cend(); ++i) {
      ostr << '"' << i->first << '"';
      ostr << ":[";
      const std::vector<double> &vec = i->second;
      for (auto j = vec.cbegin(); j != vec.cend(); ++j) {
	ostr << *j;
	if (++(std::vector<double>::const_iterator(j)) != vec.cend()) ostr << ',';
      }
      ostr << "]";
      if (++(decltype(stat_map)::const_iterator(i)) != stat_map.cend()) ostr << ',';
    }
    ostr << "}";
    ostr.flush();
  }

private:
  std::unordered_map<std::string, std::vector<double> > stat_map;
};

class stat_counter_backend :
  public boost::log::sinks::basic_sink_backend<
  boost::log::sinks::synchronized_feeding> {
public:
  void consume(const boost::log::record_view &rec) {
    boost::log::value_ref<std::string, tag::stat_stream > name = rec[stat_stream];
    if (!name) return;
    boost::log::value_ref<statistic_type, tag::stat_type > type = rec[stat_type];
    if (!type || !type.get() == counter) return;
    auto i = stat_map.find(name.get());
    if (i == stat_map.end())
      stat_map.insert(make_pair(name.get(), 1));
    else
      i->second++;
  }

  void print_to_json(std::ostream &ostr) {
    ostr << "{";
    for (auto i = stat_map.cbegin(); i != stat_map.cend(); ++i) {
      ostr << '"' << i->first << '"';
      ostr << ':';
      ostr << i->second;
      if (++(decltype(stat_map)::const_iterator(i)) != stat_map.cend()) ostr << ',';
    }
    ostr << "}";
    ostr.flush();
  }

private:
  std::unordered_map<std::string, long> stat_map;
};

class stat_backend :
  public boost::log::sinks::basic_sink_backend<
  boost::log::sinks::synchronized_feeding> {
public:
  explicit stat_backend(const boost::shared_ptr<std::ostream> &output_stream) :
    os(output_stream) {}

  ~stat_backend() {
    std::ostream &ostr = *os;
    ostr << '{';
    ostr << "\"Counters\":";
    counter_backend.print_to_json(ostr);
    ostr << ',';
    ostr << "\"Scalars\":";
    scalar_backend.print_to_json(ostr);
    ostr << '}';
    ostr.flush();
  }

  void consume(const boost::log::record_view &rec) {
    boost::log::value_ref<std::string, tag::stat_stream > name = rec[stat_stream];
    if (!name) return;
    boost::log::value_ref<statistic_type, tag::stat_type > type = rec[stat_type];
    if (!type) return;
    switch (type.get()) {
    case counter:
      counter_backend.consume(rec);
      break;
    case scalar:
      scalar_backend.consume(rec);
      break;
    }
  }

private:
  boost::shared_ptr<std::ostream> os;
  stat_scalar_backend scalar_backend;
  stat_counter_backend counter_backend;
};

inline default_logger make_stat_logger(const std::string &name,
				       statistic_type type) {
  default_logger logger;
  logger.add_attribute("StatisticStream",
		       boost::log::attributes::constant<std::string>(name));
  logger.add_attribute("StatisticType",
		       boost::log::attributes::constant<statistic_type>(type));
  return logger;
}

inline boost::shared_ptr<default_stat_sink> make_stat_sink(const boost::shared_ptr<std::ostream> &os) {
  using boost::shared_ptr;

  shared_ptr<stat_backend> backend(new stat_backend(os));
  shared_ptr<default_stat_sink> log_sink(new default_stat_sink(backend));
  log_sink->set_filter(boost::log::expressions::has_attr(stat_stream));
  return log_sink;
}

inline boost::shared_ptr<default_cerr_sink> make_cerr_sink(severity_level level) {
  using boost::shared_ptr;
  using boost::empty_deleter;
  using boost::make_shared;
  namespace sinks = boost::log::sinks;
  namespace expr = boost::log::expressions;

  shared_ptr<sinks::text_ostream_backend> backend =
    make_shared<sinks::text_ostream_backend>();
  backend->add_stream(shared_ptr<std::ostream>(&std::cerr, empty_deleter()));

  shared_ptr<default_cerr_sink> log_sink(new default_cerr_sink(backend));
  log_sink->set_filter(!expr::has_attr(stat_stream) && severity >= level);
  return log_sink;
}

#define PUT_STAT_SCALAR(lg, value)					\
  if (true) {								\
    BOOST_LOG_SEV(lg, debug) <<						\
      boost::log::add_value("StatisticValue", (double)(value));		\
  } else ((void)0)

#define PUT_STAT_COUNTER(lg)						\
  if (true) {								\
    BOOST_LOG_SEV(lg, debug);						\
  } else ((void)0)

#endif
