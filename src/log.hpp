#ifndef LOG_HPP
#define LOG_HPP

#include "packets.hpp"
#include "rng.hpp"

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/manipulators.hpp>
#include <boost/utility/empty_deleter.hpp>

#include <iostream>
//#include <memory>
#include <string>

enum severity_level
{
  debug,
  info,
  warning,
  error
};

typedef boost::log::sources::severity_logger<severity_level> default_logger;

// template <class LoggerT>
// void log_fountain_packet(LoggerT &logger, severity_level sev,
// 			 const fountain_packet &p,
// 			 const std::string &tag,
// 			 const std::string &msg) {
//   boost::log::record rec = logger.open_record(boost::log::keywords::severity = sev);
//   if (rec)
//     {
//       rec.attribute_values().insert("tag", boost::log::attributes::make_attribute_value(tag));
//       rec.attribute_values().insert("packet_block", boost::log::attributes::make_attribute_value(p.block_number()));
//       rec.attribute_values().insert("packet_seqno", boost::log::attributes::make_attribute_value(p.sequence_number()));
//       rec.attribute_values().insert("packet_seed", boost::log::attributes::make_attribute_value(p.block_seed()));
//       rec.attribute_values().insert("packet_size", boost::log::attributes::make_attribute_value(p.size()));

//       boost::log::record_ostream strm(rec);      
//       strm << msg;
//       strm.flush();
//       logger.push_record(boost::move(rec));
//     }
// }

// template <class LoggerT>
// void log_packet(LoggerT &logger, severity_level sev,
// 		const packet &p,
// 		const std::string &tag,
// 		const std::string &msg) {
//   boost::log::record rec = logger.open_record(boost::log::keywords::severity = sev);
//   if (rec)
//     {
//       rec.attribute_values().insert("tag", boost::log::attributes::make_attribute_value(tag));
//       rec.attribute_values().insert("packet_size", boost::log::attributes::make_attribute_value(p.size()));

//       boost::log::record_ostream strm(rec);      
//       strm << msg;
//       strm.flush();
//       logger.push_record(boost::move(rec));
//     }
// }

// template <class LoggerT, class DegDist>
// void log_fountain_row(LoggerT &logger, severity_level sev,
// 		      const fountain<DegDist>::row_type &row,
// 		      const std::string &tag,
// 		      const std::string &msg) {
//   boost::log::record rec = logger.open_record(boost::log::keywords::severity = sev);
//   if (rec)
//     {
//       rec.attribute_values().insert("tag", boost::log::attributes::make_attribute_value(tag));
//       rec.attribute_values().insert("fountain_row", boost::log::attributes::make_attribute_value(row));

//       boost::log::record_ostream strm(rec);      
//       strm << msg;
//       strm.flush();
//       logger.push_record(boost::move(rec));
//     }
// }


// void init_sink() {
//   using boost::empty_deleter;
//   using boost::make_shared;
//   using boost::shared_ptr;
//   using namespace std;
//   namespace logging = boost::log;
//   namespace sinks = boost::log::sinks;
//   namespace expr = boost::log::expressions;

//   auto backend = make_shared<sinks::text_ostream_backend>();
//   backend->add_stream(shared_ptr<ostream>(&cerr, empty_deleter()));
//   backend->auto_flush(true);

//   shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend> >
//     sink(new sinks::synchronous_sink<sinks::text_ostream_backend>(backend));
//   //sink->set_filter();
//   //sink->set_formatter(expr::stream << "Hi");

//   shared_ptr<logging::core> core = logging::core::get();
//   core->add_sink(sink);
// }

#endif
