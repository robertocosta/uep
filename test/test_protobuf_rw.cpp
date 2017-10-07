#define BOOST_TEST_MODULE test_protobuf_rw
#include <boost/test/unit_test.hpp>

#include <random>
#include <cmath>

#include "protobuf_rw.hpp"
#include "controlMessage.pb.h"

using namespace boost::asio;
using namespace std;
using namespace uep;
using namespace uep::net;
using namespace uep::protobuf;

using boost::asio::ip::tcp;

// Set globally the log severity level
struct global_fixture {
  global_fixture() {
    uep::log::init();
    //auto warn_filter = boost::log::expressions::attr<
    //  uep::log::severity_level>("Severity") >= uep::log::warning;
    //boost::log::core::get()->set_filter(warn_filter);
  }

  ~global_fixture() {
  }
};
BOOST_GLOBAL_FIXTURE(global_fixture);

struct setup_sockets {
  io_service io;
  tcp::socket sock1{io};
  tcp::socket sock2{io};
  tcp::acceptor acc{io};

  std::function<void(const boost::system::error_code&)> run_server, run_client;

  protobuf_reader prd{io, sock2};
  protobuf_writer pwr{io, sock1};

  setup_sockets() {
    acc.open(tcp::v4());
    tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8989);
    acc.bind(ep);
    acc.listen();
    acc.async_accept(sock1, [this](const auto &a){run_server(a);});
    sock2.open(tcp::v4());
    sock2.async_connect(ep, [this](const auto &a){run_client(a);});
  }
};

BOOST_FIXTURE_TEST_CASE(single_message, setup_sockets) {
  std::string sn(8*1024, 'a');

  ControlMessage cm;
  cm.set_stream_name(sn);

  ControlMessage recv;

  run_server = [&,this](const auto &ec) {
    pwr.async_write_one(cm);
  };

  run_client = [&,this](const auto &ec) {
    prd.async_read_one(recv, [&,this](const auto &ec,
				      std::size_t bytes) {
			 BOOST_CHECK_EQUAL(bytes, cm.ByteSize());
			 BOOST_CHECK(recv.stream_name() == cm.stream_name());
		       });
  };

  io.run();
}

BOOST_FIXTURE_TEST_CASE(random_message_list, setup_sockets) {
  std::vector<char> orig(1024*1024); // Send 1 MByte total
  std::default_random_engine rng;
  std::uniform_int_distribution<unsigned char> unif(32,126);
  for (char &c : orig) {
    c = static_cast<char>(unif(rng));
  }

  std::vector<ControlMessage> orig_msgs;
  auto i = orig.cbegin();
  std::uniform_int_distribution<std::size_t> unif_size(1, 16*1024);
  while (i != orig.cend()) {
    std::size_t s = std::min(unif_size(rng),
			     static_cast<std::size_t>(orig.cend() - i));
    orig_msgs.emplace_back();
    ControlMessage &m = orig_msgs.back();
    m.set_stream_name(std::string(i, i + s));
    i += s;
  }

  std::size_t sent_size = 0;
  auto j = orig_msgs.cbegin();
  uep::net::handler_type server_loop;
  server_loop = [&,this](const auto &ec, std::size_t sent_bytes) {
    BOOST_CHECK(!ec);
    if (j != orig_msgs.cbegin()) {
      BOOST_CHECK_EQUAL(sent_bytes, (j-1)->ByteSize());
    }
    if (j == orig_msgs.cend()) return;
    sent_size += j->stream_name().size();
    pwr.async_write_one(*j++, server_loop);
  };

  run_server = [&,this](const auto &ec) {
    BOOST_CHECK(!ec);
    server_loop(boost::system::error_code(), 0);
  };

  std::size_t recv_size = 0;
  std::vector<ControlMessage> recv_msgs;
  uep::net::handler_type client_loop;
  client_loop = [&,this](const auto &ec, std::size_t recv_bytes) {
    BOOST_CHECK(!ec);
    if (recv_msgs.size() != 0) {
      recv_size += (recv_msgs.end()-1)->stream_name().size();
    }
    if (recv_msgs.size() == orig_msgs.size()) return;
    recv_msgs.emplace_back();
    ControlMessage &m = recv_msgs.back();
    prd.async_read_one(m, client_loop);
  };

  run_client = [&,this](const auto &ec) {
    BOOST_CHECK(!ec);
    client_loop(boost::system::error_code(), 0);
  };

  io.run();

  BOOST_CHECK_EQUAL(sent_size, 1024*1024);
  BOOST_CHECK_EQUAL(recv_size, 1024*1024);

  for (size_t n = 0; n < orig_msgs.size(); ++n) {
    BOOST_CHECK(orig_msgs[n].stream_name() == recv_msgs[n].stream_name());
  }

  std::vector<char> recv;
  std::for_each(recv_msgs.cbegin(), recv_msgs.cend(),
		[&recv](const auto &m) {
		  recv.insert(recv.end(),
			      m.stream_name().begin(),
			      m.stream_name().end());
		});
  BOOST_CHECK_EQUAL(orig.size(), recv.size());
  BOOST_CHECK_EQUAL(orig.size(), sent_size);
  BOOST_CHECK(std::equal(orig.cbegin(), orig.cend(), recv.cbegin()));
}
