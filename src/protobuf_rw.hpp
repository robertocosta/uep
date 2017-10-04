#ifndef PROTOBUF_RW_HPP
#define PROTOBUF_RW_HPP

#include <functional>
#include <istream>

#include <boost/asio.hpp>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/message_lite.h>

#include "log.hpp"
#include "rw_utils.hpp"

namespace uep { namespace net {

/** Type of the handlers to be passed to protobuf_reader and
 *  protobuf_writer.
 */
using handler_type = std::function<void(const boost::system::error_code&,
					std::size_t)>;
/** Integer type used to store the length of the messages. */
using msglen_type = std::uint16_t;

/** Class to read a sequence of framed protobuf messages of type M
 *  from a TCP socket.
 */
class protobuf_reader {
  using msg_type = google::protobuf::MessageLite;

public:
  explicit protobuf_reader(boost::asio::io_service &io_svc,
			   boost::asio::ip::tcp::socket &sock);

  /** Read one framed message from the TCP socket and call the
   *  handler. The handler must be a function like `void h(const
   *  boost::system::error_code &ec, std::size_t msg_length)`.
   */
  void async_read_one(msg_type &out, const handler_type &hnd);

private:
  log::default_logger basic_lg, perf_lg;

  boost::asio::ip::tcp::socket &socket; /**< TCP socket used to read. */
  boost::asio::io_service::strand strand; /**< Strand used to
					   *   serialize the handlers.
					   */

  boost::asio::streambuf in_buf; /**< Buffer used to hold the received
				  *	 data.
				  */
  std::istream in_str; /**< Input stream tied to the in_buf buffer. */
  std::size_t exp_msg_len; /**< Length expected for the next message to be read. */

  handler_type handler; /**< The handler to be called when a new
			 *   framed message is received.
			 */
  msg_type *out_msg; /**< Pointer to the output message. */

  bool readall; /**< Used to distinguish async_read_one from
		 *   async_read_all.
		 */

  /** Read from the TCP socket into the in_buf buffer. */
  void wait_message();
  /** Handle the TCP read: unframe the message and call the handler or
   *  read more data.
   */
  void handle_read(const boost::system::error_code &ec,
		   std::size_t bytes_rx);
};

/** Class to write a sequence of framed protobuf messages to a TCP
 *  socket.
 */
class protobuf_writer {
  using msg_type = google::protobuf::MessageLite;

public:
  explicit protobuf_writer(boost::asio::io_service &io_svc,
			   boost::asio::ip::tcp::socket &sock);

  /** Frame and write the given message, then call the handler. */
  void async_write_one(const msg_type &in, const handler_type &hnd);
  /** Frame and write the given message. */
  void async_write_one(const msg_type &in);

private:
  log::default_logger basic_lg, perf_lg;

  boost::asio::ip::tcp::socket &socket;
  boost::asio::io_service::strand strand;

  std::vector<char> in_buf; /**< Encoded message to be sent. */
  handler_type handler;

  void send_length();
  void send_data(const boost::system::error_code &ec, std::size_t len_sent);
  void handle_sent(const boost::system::error_code &ec, std::size_t data_sent);
};

}}

#endif
