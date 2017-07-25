#define BOOST_TEST_MODULE test_data_client_server
#include <iostream>

#include <boost/test/unit_test.hpp>

#include "data_client_server.hpp"
#include "encoder.hpp"
#include "decoder.hpp"

/* Produce random packets of fixed size.
 * Parameters: rng seed, pkt size, stop after max_count pkts
 */
struct random_packet_source {
  struct parameter_set {
    std::mt19937::result_type seed;
    std::size_t size;
    std::size_t max_count;
  };

  std::independent_bits_engine<std::mt19937, CHAR_BIT, unsigned char> rng;
  std::size_t size;
  std::size_t max_count;
  std::size_t count;

  std::vector<packet> original;

  explicit random_packet_source(const parameter_set &ps) :
    rng(ps.seed), size(ps.size), max_count(ps.max_count), count(0) {};

  packet next_packet() {
    if (count == max_count) throw std::runtime_error("Max packet count");
    else ++count;
    packet p;
    p.resize(size);
    for (std::size_t i=0; i < size; i++) {
      p[i] = rng();
    }
    original.push_back(p);
    return p;
  }

  explicit operator bool() const {
    return count < max_count;
  }

  bool operator!() const { return !static_cast<bool>(*this); }
};

/* Store the received packets in a vector
 * No paramters
 * Can access publicly the vector
 */
struct memory_sink {
  struct parameter_set {};
  explicit memory_sink(const parameter_set&) {}

  std::vector<packet> received;

  void push(const packet &p) {
    packet p_copy(p);
    using std::move;
    push(move(p));
  }

  void push(packet &&p) {
    using std::move;
    received.push_back(move(p));
  }

  explicit operator bool() const { return true; }
  bool operator!() const { return false; }
};

using namespace std;
using namespace uep;
using namespace uep::net;

using namespace boost::asio;

struct setup_cs {
  io_service io; // Global io_service object

  std::size_t L = 1024; // pkt size

  lt_encoder<std::mt19937>::parameter_set enc_ps;
  lt_decoder::parameter_set dec_ps;
  random_packet_source::parameter_set src_ps;
  memory_sink::parameter_set sink_ps;

  data_server<lt_encoder<std::mt19937>,random_packet_source> ds;
  data_client<lt_decoder,memory_sink> dc;

  setup_cs() :
    enc_ps{100, 0.1, 0.5}, // parametrs of the encoder/decoder (K = 100, c = 0.1, delta = 0.5)
    dec_ps{100, 0.1, 0.5},
    src_ps{0x42, L, 100}, // generate K packets of size L, fixed seed
    ds(io), // construct the data_server
    dc(io) { // construct the data_client
      ds.setup_encoder(enc_ps); // setup the encoder inside the data_server
      ds.setup_source(src_ps); // setup the source  inside the data_server
      ds.target_send_rate(10240); // Set a target send rate of 10240 byte/s = 10 pkt/s
      ds.open("127.0.0.1", "9999"); // Setup the data_server socket: random_port -> 127.0.0.1:9999



      BOOST_CHECK_EQUAL(src_ps.seed, 0x42);
      BOOST_CHECK_EQUAL(src_ps.size, L);
      BOOST_CHECK_EQUAL(src_ps.max_count, 100);

      dc.setup_decoder(dec_ps); // setup the decoder inside the data_client
      dc.setup_sink(sink_ps); // setup the sink inside the data_client
      dc.bind("9999"); // bind the data_client to the port 9999
  }
};

BOOST_FIXTURE_TEST_CASE(test_send, setup_cs) {
  // After setup_cs::setup_cs() ...

  //BOOST_CHECK(enc.has_block());

  auto srv_ep = ds.server_endpoint(); // get the random source port used by the data_server
  dc.start_receive(srv_ep); // schdule the data_client to listen on its bound port for packets from the server's ip:port

  // schedule a stop after 30 seconds
  boost::asio::steady_timer end_timer(io, std::chrono::seconds(30));
  end_timer.async_wait([this]
		       (const boost::system::error_code&) -> void {
			 //cerr << "Stopping" << endl;
			 ds.stop();
			 dc.close();
		       });
  ds.start(); // start the periodic tx of packets
  io.run(); // enter the io_service loop: process both the server and the client scheduled actions until they stop (empty action queue)

  const std::vector<packet> &orig = ds.source().original;
  const std::vector<packet> &recv = dc.sink().received;

  // verify number of packets
  BOOST_CHECK_EQUAL(recv.size(), 100); // one fully decoded block
  BOOST_CHECK_EQUAL(orig.size(), 100); // one block was extracted
  // verify correct reception
  BOOST_CHECK(equal(recv.cbegin(), recv.cend(), orig.cbegin()));
}

BOOST_AUTO_TEST_CASE(send_with_ack) {
  using namespace std;

  io_service io; // Global io_service object

  const size_t L = 1024; // pkt size
  const size_t K = 100; // block size
  const double c = 0.1;
  const double delta = 0.5;
  const size_t N = 10*K; // total packets to send
  const mt19937::result_type src_seed = 0x42;

  lt_encoder<std::mt19937>::parameter_set enc_ps{K,c,delta};
  lt_decoder::parameter_set dec_ps = enc_ps;
  random_packet_source::parameter_set src_ps{src_seed,L,N};
  memory_sink::parameter_set sink_ps;

  data_server<lt_encoder<std::mt19937>,random_packet_source> ds(io);
  data_client<lt_decoder,memory_sink> dc(io);

  ds.setup_encoder(enc_ps); // setup the encoder inside the data_server
  ds.setup_source(src_ps); // setup the source  inside the data_server
  //ds.target_send_rate(L); // Set a target send rate of 1 pkt/s
  ds.open("127.0.0.1", "9999"); // Setup the data_server socket:
				// random_port -> 127.0.0.1:9999

  dc.setup_decoder(dec_ps); // setup the decoder inside the data_client
  dc.setup_sink(sink_ps); // setup the sink inside the data_client
  dc.bind("9999"); // bind the data_client to the port 9999

  auto srv_ep = ds.server_endpoint(); // get the random source port
				      // used by the data_server
  dc.start_receive(srv_ep); // schdule the data_client to listen on
			    // its bound port for packets from the
			    // server's ip:port

  ds.start(); // start the periodic tx of packets
  // schedule a stop after some time
  boost::asio::steady_timer end_timer(io, std::chrono::seconds(30));
  end_timer.async_wait([&ds,&dc]
		       (const boost::system::error_code&) -> void {
			 ds.stop();
			 dc.close();
		       });
    io.run(); // enter the io_service loop: process both the server and
	    // the client scheduled actions until they stop (empty
	    // action queue)

  const std::vector<packet> &orig = ds.source().original;
  const std::vector<packet> &recv = dc.sink().received;

  // verify number of packets
  BOOST_CHECK_EQUAL(recv.size(), N); // one fully decoded block
  BOOST_CHECK_EQUAL(orig.size(), N); // one block was extracted
  // verify correct reception
  BOOST_CHECK(equal(recv.cbegin(), recv.cend(), orig.cbegin()));
}
