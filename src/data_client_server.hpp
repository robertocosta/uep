#ifndef UEP_NET_DATA_CLIENT_SERVER_HPP
#define UEP_NET_DATA_CLIENT_SERVER_HPP

#include <chrono>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include "packets_rw.hpp"

namespace uep {
namespace net {

template <class Decoder, class Sink>
class data_client {
  static const std::size_t UDP_MAX_PAYLOAD = 0x10000;

public:
  typedef Decoder decoder_type;
  typedef Sink sink_type;
  typedef typename Decoder::parameter_set decoder_parameter_set;
  typedef typename Sink::parameter_set sink_parameter_set;

  explicit data_client(boost::asio::io_service &io) :
    io_service_(io), strand_(io_service_), socket_(io_service_),
    recv_buffer(UDP_MAX_PAYLOAD) {
  }

  void setup_decoder(const decoder_parameter_set &ps) {
    decoder_.reset(new Decoder(ps));
  }

  void setup_sink(const sink_parameter_set &ps) {
    sink_.reset(new Sink(ps));
  }

  void bind(const std::string &service) {
    using boost::asio::ip::udp;
    udp::resolver resolver(io_service_);
    udp::resolver::query listen_q(service);
    listen_endpoint_ = *resolver.resolve(listen_q);
    socket_.open(udp::v4());
    socket_.bind(listen_endpoint_);
  }

  void start_receive(const boost::asio::ip::udp::endpoint &src_ep) {
    using namespace std::placeholders;
    server_endpoint_ = src_ep;
    socket_.async_receive_from(boost::asio::buffer(recv_buffer),
			       server_endpoint_,
			       std::bind(&data_client::handle_received,
					 this,
					 _1, _2));
  }

  void start_receive(const std::string &src_addr, unsigned short src_port) {
    using namespace boost::asio;
    ip::address a = ip::address::from_string(src_addr);
    start_receive(ip::udp::endpoint(a, src_port));
  }

  void close() {
    strand_.dispatch(std::bind(&data_client::handle_close, this));
  }

  const Sink &sink() const {
    return *sink_;
  }

private:
  std::unique_ptr<Decoder> decoder_; // could not be default-constructible
  std::unique_ptr<Sink> sink_; // could not be default-constructible

  boost::asio::io_service &io_service_;
  boost::asio::io_service::strand strand_;
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint listen_endpoint_;
  boost::asio::ip::udp::endpoint server_endpoint_;

  std::vector<char> recv_buffer;

  void handle_received(const boost::system::error_code& ec, std::size_t size) {
    if (ec == boost::asio::error::operation_aborted) return;
    if (ec) throw boost::system::system_error(ec);
    if (size == 0) throw std::runtime_error("Empty packet");
    recv_buffer.resize(size);
    fountain_packet p = parse_raw_data_packet(recv_buffer);
    using std::move;
    decoder_->push(move(p));

    while (*decoder_ && *sink_) {
      sink_->push(decoder_->next_decoded());
    }

    using namespace std::placeholders;
    socket_.async_receive_from(boost::asio::buffer(recv_buffer),
			    server_endpoint_,
			    std::bind(&data_client::handle_received,
				      this,
				      _1, _2));
  }

  void handle_close() {
    socket_.cancel();
    socket_.close();
  }
};

/** Send the packets output by an encoder through a UDP socket.
 *  This class wraps the Encoder to provide thread-safety, the Encoder
 *  class must have a constructor that takes a const
 *  Encoder::parameter_set&, a push(packet&&) and push(const packet&)
 *  to provide the input data, a next_coded() that returns a coded
 *  packet, and a next_block() that proceeds to the next block of
 *  packets.
 */
template <class Encoder, class Source>
class data_server {
public:
  typedef Encoder encoder_type;
  typedef Source source_type;
  typedef typename Encoder::parameter_set encoder_parameter_set;
  typedef typename Source::parameter_set source_parameter_set;

  explicit data_server(boost::asio::io_service &io) :
    io_service_(io),
    strand_(io_service_),
    socket_(io_service_),
    target_send_rate_(std::numeric_limits<double>::infinity()),
    is_stopped_(true),
    more_data_(false),
    pkt_timer(io_service_) {
  }

  void setup_encoder(const encoder_parameter_set &ps) {
    encoder_.reset(new Encoder(ps));
  }

  void setup_source(const source_parameter_set &ps) {
    source_.reset(new Source(ps));
    more_data_ = static_cast<bool>(*source_);
  }

  void open(const std::string &dest_host, const std::string &dest_service) {
    using boost::asio::ip::udp;
    udp::resolver resolver(io_service_);
    udp::resolver::query query(udp::v4(), dest_host, dest_service);
    client_endpoint_ = *resolver.resolve(query);

    socket_.open(udp::v4());
  }

  void start() {
    strand_.dispatch(std::bind(&data_server::handle_started, this));
  }

  void stop() {
    strand_.dispatch(std::bind(&data_server::handle_stopped, this));
  }

  void next_block() {
    strand_.dispatch(std::bind(&Encoder::next_block, encoder_.get()));
  }

  void target_send_rate(double sr) {
    target_send_rate_ = sr;
  }

  double target_send_rate() const {
    return target_send_rate_;
  }

  boost::asio::ip::udp::endpoint server_endpoint() const {
    return socket_.local_endpoint();
  }

  const Source &source() const {
    return *source_;
  }

private:
  std::unique_ptr<Encoder> encoder_;
  std::unique_ptr<Source> source_;

  boost::asio::io_service &io_service_;
  boost::asio::io_service::strand strand_;
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint client_endpoint_;

  std::atomic<double> target_send_rate_;

  bool is_stopped_;
  bool more_data_;
  std::vector<char> last_pkt;
  std::chrono::steady_clock::time_point last_sent_time;
  boost::asio::steady_timer pkt_timer;

  void schedule_next_pkt() {
    if (is_stopped_) return;

    while (*source_ && !*encoder_) {
      encoder_->push(source_->next_packet());
    }

    if (!*source_) {
      is_stopped_ = true;
      more_data_ = false;
      return;
    }

    using std::move;
    fountain_packet p = encoder_->next_coded();
    bool is_first = last_pkt.empty();
    last_pkt = build_raw_packet(move(p));

    using std::chrono::seconds;
    using std::chrono::microseconds;
    if (is_first) {
      pkt_timer.expires_from_now(seconds(0));
    }
    else {
      double sr = target_send_rate_;
      decltype(last_sent_time) next_send = last_sent_time +
	microseconds(static_cast<long>(1e6 / sr * last_pkt.size()));
      pkt_timer.expires_at(next_send);
    }

    using namespace std::placeholders;
    pkt_timer.async_wait(strand_.wrap(std::bind(&data_server::handle_send_timer,
						 this, _1)));
  }

  void handle_send_timer(const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) return;
    if (ec) throw boost::system::system_error(ec);

    using namespace std::placeholders;
    socket_.async_send_to(boost::asio::buffer(last_pkt),
			  client_endpoint_,
			  strand_.wrap(std::bind(&data_server::handle_sent,
						 this,
						 _1, _2)));
  }

  void handle_sent(const boost::system::error_code &ec,
		   std::size_t sent_size) {
    if (ec) throw boost::system::system_error(ec);
    if (sent_size != last_pkt.size())
      throw std::runtime_error("Did not send all the packet");
    last_sent_time = std::chrono::steady_clock::now();

    schedule_next_pkt();
  }

  void handle_started() {
    is_stopped_ = false;
    schedule_next_pkt();
  }

  void handle_stopped() {
    is_stopped_ = true;
    pkt_timer.cancel();
  }
};

}
}

#endif
