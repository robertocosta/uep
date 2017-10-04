#include "protobuf_rw.hpp"

namespace uep { namespace net {

protobuf_reader::protobuf_reader(boost::asio::io_service &io_svc,
				 boost::asio::ip::tcp::socket &sock) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  socket(sock),
  strand(io_svc),
  in_str(&in_buf),
  exp_msg_len(0) {
}

void protobuf_reader::async_read_one(msg_type &out, const handler_type &hnd) {
  readall = false;
  out_msg = &out;
  handler = hnd;
  BOOST_LOG_SEV(basic_lg, log::trace) << "Start to receive one message";
  wait_message();
}

void protobuf_reader::wait_message() {
  using namespace std::placeholders;

  BOOST_LOG_SEV(basic_lg, log::trace) << "Wait for a TCP read";

  auto rd_bufs = in_buf.prepare(4096);
  auto hnd = strand.wrap(std::bind(&protobuf_reader::handle_read,
				   this, _1, _2));
  socket.async_read_some(rd_bufs, hnd);
}

void protobuf_reader::handle_read(const boost::system::error_code &ec,
				  std::size_t bytes_rx) {
  using namespace std::placeholders;
  using namespace boost::asio;
  using namespace boost::system;

  using google::protobuf::io::IstreamInputStream;

  BOOST_LOG_SEV(basic_lg, log::trace) << "Finished a TCP read with "
				      << bytes_rx << " bytes";

  if (ec) { // Forward the error code to the handler
    BOOST_LOG_SEV(basic_lg, log::trace) << "Nonzero error code: " << ec;
    strand.dispatch(std::bind(handler, ec, 0));
    return;
  }

  in_buf.commit(bytes_rx); // Move the received bytes to the input buffer

  if (exp_msg_len == 0) { // New message: read length
    BOOST_LOG_SEV(basic_lg, log::trace) << "New message";
    // Too few bytes to read the length: read again
    if (static_cast<std::size_t>(in_buf.in_avail()) <
	sizeof(msglen_type)) {
      BOOST_LOG_SEV(basic_lg, log::trace) << "Too short: read more";
      wait_message();
      return;
    }

    // Read the prefix message length
    msglen_type len;
    rw_utils::read_ntoh<msglen_type>(len, in_str);
    exp_msg_len = len;
    BOOST_LOG_SEV(basic_lg, log::trace) << "Expecting a message of "
					<< exp_msg_len << " bytes";
  }

  if (static_cast<std::size_t>(in_buf.in_avail()) >=
      exp_msg_len) { // Can read a whole message
    BOOST_LOG_SEV(basic_lg, log::trace) << "Read a whole message";
    std::vector<char> msg_buf(exp_msg_len);
    in_str.read(msg_buf.data(), exp_msg_len);
    if (!out_msg->ParseFromArray(msg_buf.data(), exp_msg_len)) {
      throw std::runtime_error("Message parsing failed");
    }

    // Schedule a handler for the new message
    BOOST_LOG_SEV(basic_lg, log::trace) << "Call the handler";
    strand.dispatch(std::bind(handler, ec, exp_msg_len));
    exp_msg_len = 0; // Expect a new message
    if (!readall) return; // Was async_read_one?
  }

  // Read more
  BOOST_LOG_SEV(basic_lg, log::trace) << "Read more";
  wait_message();
}

protobuf_writer::protobuf_writer(boost::asio::io_service &io_svc,
				 boost::asio::ip::tcp::socket &sock) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  socket(sock),
  strand(io_svc) {
}

void protobuf_writer::async_write_one(const msg_type &in, const handler_type &hnd) {
  in_buf.resize(in.ByteSize());
  BOOST_LOG_SEV(basic_lg, log::trace) << "Send a message of "
				      << in_buf.size() << " bytes";
  if (!in.SerializeToArray(in_buf.data(), in_buf.size())) {
    throw std::runtime_error("Message serialization failed");
  }
  handler = hnd;
  send_length();
}

void protobuf_writer::async_write_one(const msg_type &in) {
  async_write_one(in, [](const boost::system::error_code &ec,
			 std::size_t){
		    if (ec) throw boost::system::system_error(ec);
		  });
}

void protobuf_writer::send_length() {
  using namespace boost::asio;
  using namespace std::placeholders;

  msglen_type len = static_cast<msglen_type>(in_buf.size());
  if (len == 0) throw std::runtime_error("Empty buffer");
  std::array<char, sizeof(msglen_type)> rawlen;
  rw_utils::write_hton<msglen_type>(len, rawlen.begin(), rawlen.end());
  auto h = strand.wrap(std::bind(&protobuf_writer::send_data,
				 this, _1, _2));
  BOOST_LOG_SEV(basic_lg, log::trace) << "Send " << sizeof(msglen_type)
				      << " bytes of length";

  async_write(socket, buffer(rawlen), h);
}

void protobuf_writer::send_data(const boost::system::error_code &ec,
				std::size_t len_sent) {
  using namespace std::placeholders;

  if (ec) { // Forward the error code to the handler
    BOOST_LOG_SEV(basic_lg, log::trace) << "Nonzero error code: " << ec;
    strand.dispatch(std::bind(handler, ec, 0));
    return;
  }
  if (len_sent != sizeof(msglen_type)) {
    throw std::range_error("sent a wrong number of bytes for the length");
  }

  auto h = strand.wrap(std::bind(&protobuf_writer::handle_sent,
				 this, _1, _2));
  boost::asio::async_write(socket, boost::asio::buffer(in_buf), h);
}

void protobuf_writer::handle_sent(const boost::system::error_code &ec,
				  std::size_t data_sent) {
  if (ec) { // Forward the error code to the handler
    BOOST_LOG_SEV(basic_lg, log::trace) << "Nonzero error code: " << ec;
    strand.dispatch(std::bind(handler, ec, 0));
    return;
  }
  if (data_sent != in_buf.size()) {
    throw std::range_error("sent a wrong number of bytes for the data");
  }

  strand.dispatch(std::bind(handler, ec, data_sent));
}

}}
