#include <string>
#include <iostream>
#include <sstream>

#include "data_client_server.hpp"
#include "decoder.hpp"

using namespace uep;
using namespace uep::net;

struct data_parameter_set {
  bool enable_ack;
  std::size_t expected_pkt_count;
  double target_send_rate;
  std::size_t max_sequence_number;
  double timeout;
};

struct cout_pkt_sink {
  struct parameter_set {};

  explicit cout_pkt_sink(const parameter_set&) {}

  std::vector<packet> received;

  void push(const packet &p) {
    packet p_copy(p);
    using std::move;
    push(move(p_copy));
  }

  void push(packet &&p) {
    using std::move;
    if (p)
      std::cout << p.data() << std::endl;
    else
      std::cout << "... Failed ..." << std::endl;
    received.push_back(move(p));
  }

  explicit operator bool() const { return true; }
  bool operator!() const { return false; }
};

const std::size_t nblocks = 2, K = 20000;
const robust_lt_parameter_set lt_par{K, 0.1, 0.5};
const data_parameter_set data_par{true,
    nblocks*K,
    0,
    (size_t)(1.2*K),
    60};

int main(int argc, char **argv) {
  using namespace uep::log;

  log::init("demo_dc.log");
  default_logger basic_lg(boost::log::keywords::channel = basic);
  default_logger perf_lg(boost::log::keywords::channel = performance);

  boost::asio::io_service io;

  data_client<lt_decoder,cout_pkt_sink> dc(io);
  dc.setup_decoder(lt_par);
  dc.setup_sink(cout_pkt_sink::parameter_set());
  dc.enable_ack(data_par.enable_ack);
  dc.expected_count(data_par.expected_pkt_count);
  dc.timeout(data_par.timeout);

  std::string port;
  std::string srv_ip;
  std::string srv_port;
  if (argc == 4) {
    port = argv[1];
    srv_ip = argv[2];
    srv_port = argv[3];
  }
  else {
    std::cerr << "Give 3 args" << std::endl;
    return 1;
  }

  dc.bind(port);
  dc.start_receive(srv_ip, srv_port);

  BOOST_LOG_SEV(basic_lg, debug) << "Start io_service loop";
  io.run();
  BOOST_LOG_SEV(basic_lg, debug) << "io_service loop ended";
  return 0;
}
