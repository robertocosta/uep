#define BOOST_TEST_MODULE test_data_client_server
#include <boost/test/unit_test.hpp>

#include "data_client_server.hpp"
#include "decoder.hpp"
#include "encoder.hpp"
#include "uep_decoder.hpp"
#include "uep_encoder.hpp"

// Set globally the log severity level
struct global_fixture {
  global_fixture() {
    uep::log::init();
    auto warn_filter = boost::log::expressions::attr<
      uep::log::severity_level>("Severity") >= uep::log::warning;
    boost::log::core::get()->set_filter(warn_filter);
  }

  ~global_fixture() {
  }
};
BOOST_GLOBAL_FIXTURE(global_fixture);

/* Produce random packets of fixed size following the given subblock
 * configuration.  Parameters: rng seed, pkt size, stop after
 * max_count pkts
 */
struct random_packet_source {
  struct parameter_set {
    std::mt19937::result_type seed;
    std::size_t size;
    std::size_t max_count;
    std::vector<std::size_t> Ks;
  };

  std::independent_bits_engine<std::mt19937, CHAR_BIT, unsigned char> rng;
  std::size_t size;
  std::size_t max_count;
  std::size_t count;
  std::vector<std::size_t> Ks;

  std::vector<packet> original;
  std::vector<fountain_packet> original_fp;

  explicit random_packet_source(const parameter_set &ps) :
    rng(ps.seed), size(ps.size), max_count(ps.max_count), count(0),
    Ks(ps.Ks) {
    if (Ks.empty()) {
      Ks = {1};
    }
  }

  std::size_t subblock(std::size_t cnt) {
    cnt %= std::accumulate(Ks.begin(), Ks.end(), 0);
    std::size_t sb = 0;
    std::size_t cum_K = 0;
    while (cum_K + Ks[sb] <= cnt) {
      cum_K += Ks[sb];
      ++sb;
    }
    return sb;
  }

  fountain_packet next_packet() {
    if (count == max_count) throw std::runtime_error("Max packet count");

    fountain_packet p;
    p.resize(size);
    for (std::size_t i=0; i < size; i++) {
      p[i] = rng();
    }
    p.setPriority(subblock(count));
    original.push_back(p);
    original_fp.push_back(p);
    ++count;
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
  std::vector<fountain_packet> received_fp;

  void push(const fountain_packet &p) {
    received.push_back(p);
    received_fp.push_back(p);
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
      ds.enable_ack(false);
      ds.open("127.0.0.1", "9999"); // Setup the data_server socket: random_port -> 127.0.0.1:9999



      BOOST_CHECK_EQUAL(src_ps.seed, 0x42);
      BOOST_CHECK_EQUAL(src_ps.size, L);
      BOOST_CHECK_EQUAL(src_ps.max_count, 100);

      dc.setup_decoder(dec_ps); // setup the decoder inside the data_client
      dc.setup_sink(sink_ps); // setup the sink inside the data_client
      dc.enable_ack(false);
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
			 dc.stop();
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
			 dc.stop();
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

BOOST_AUTO_TEST_CASE(send_with_pkt_limit) {
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
  //ds.target_send_rate(L*2); // Set a target send rate of 2 pkt/s
  ds.enable_ack(false);
  ds.max_sequence_number(90);
  ds.open("127.0.0.1", "9999"); // Setup the data_server socket:
				// random_port -> 127.0.0.1:9999

  dc.setup_decoder(dec_ps); // setup the decoder inside the data_client
  dc.setup_sink(sink_ps); // setup the sink inside the data_client
  dc.enable_ack(false);
  dc.bind("9999"); // bind the data_client to the port 9999

  auto srv_ep = ds.server_endpoint(); // get the random source port
				      // used by the data_server
  dc.start_receive(srv_ep); // schdule the data_client to listen on
			    // its bound port for packets from the
			    // server's ip:port

  ds.start(); // start the periodic tx of packets
  // schedule a stop after some time
  boost::asio::steady_timer end_timer(io, std::chrono::seconds(10));
  end_timer.async_wait([&ds,&dc]
		       (const boost::system::error_code&) -> void {
			 ds.stop();
			 dc.stop();
		       });
    io.run(); // enter the io_service loop: process both the server and
	    // the client scheduled actions until they stop (empty
	    // action queue)

  const std::vector<packet> &orig = ds.source().original;
  const std::vector<packet> &recv = dc.sink().received;

  // verify number of packets
  BOOST_CHECK_EQUAL(orig.size(), N); // all pkts were extracted
  BOOST_CHECK(recv.size() >= N-K); // at least all blocks except the
				   // last one

  BOOST_CHECK_EQUAL(dc.decoder().blockno(), 9); // synced on the right block
  BOOST_CHECK_EQUAL(dc.decoder().received_count(), 90); // got max amount of pkts

  // Check correctness
  std::size_t count_ok = 0;
  std::size_t count_fail = 0;
  auto i = recv.cbegin();
  auto j = orig.cbegin();
  while (i != recv.cend()) {
    BOOST_CHECK(j != orig.cend());
    if (*i) {
      BOOST_CHECK(*i == *j);
      ++count_ok;
    }
    else {
      ++count_fail;
    }
    ++i;;
    ++j;
  }

  BOOST_CHECK_EQUAL(count_fail, dc.decoder().total_failed_count());
  BOOST_CHECK_EQUAL(count_ok, dc.decoder().total_decoded_count());
}

BOOST_AUTO_TEST_CASE(autoclose_client) {
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
  //ds.target_send_rate(L*2); // Set a target send rate of 2 pkt/s
  ds.enable_ack(false);
  ds.max_sequence_number(200);
  ds.open("127.0.0.1", "9999"); // Setup the data_server socket:
				// random_port -> 127.0.0.1:9999

  dc.setup_decoder(dec_ps); // setup the decoder inside the data_client
  dc.setup_sink(sink_ps); // setup the sink inside the data_client
  dc.enable_ack(false);
  dc.expected_count(N);
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
			 if (!ds.is_stopped()) {
			   ds.stop();
			   BOOST_ERROR("data_server did not stop");
			 }
			 if (!dc.is_stopped()) {
			   dc.stop();
			   BOOST_ERROR("data_client did not stop");
			 }
		       });
    io.run(); // enter the io_service loop: process both the server and
	    // the client scheduled actions until they stop (empty
	    // action queue)

  const std::vector<packet> &orig = ds.source().original;
  const std::vector<packet> &recv = dc.sink().received;

  // verify number of packets
  BOOST_CHECK_EQUAL(orig.size(), N); // all pkts were extracted
  BOOST_CHECK(recv.size() >= N-K); // at least all blocks except the
				   // last one

  BOOST_CHECK_EQUAL(dc.decoder().blockno(), 9); // synced on the right block
  BOOST_CHECK_GE(dc.decoder().received_count(), K); // got max amount of pkts

  // Check correctness
  std::size_t count_ok = 0;
  std::size_t count_fail = 0;
  auto i = recv.cbegin();
  auto j = orig.cbegin();
  while (i != recv.cend()) {
    BOOST_CHECK(j != orig.cend());
    if (*i) {
      BOOST_CHECK(*i == *j);
      ++count_ok;
    }
    else {
      ++count_fail;
    }
    ++i;;
    ++j;
  }

  BOOST_CHECK_EQUAL(count_fail, dc.decoder().total_failed_count());
  BOOST_CHECK_EQUAL(count_ok, dc.decoder().total_decoded_count());
}

BOOST_AUTO_TEST_CASE(autoclose_client_ack) {
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
  //ds.target_send_rate(L*2); // Set a target send rate of 2 pkt/s
  ds.enable_ack(true);
  ds.open("127.0.0.1", "9999"); // Setup the data_server socket:
				// random_port -> 127.0.0.1:9999

  dc.setup_decoder(dec_ps); // setup the decoder inside the data_client
  dc.setup_sink(sink_ps); // setup the sink inside the data_client
  dc.enable_ack(true);
  dc.expected_count(N);
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
			 if (!ds.is_stopped()) {
			   ds.stop();
			   BOOST_ERROR("data_server did not stop");
			 }
			 if (!dc.is_stopped()) {
			   dc.stop();
			   BOOST_ERROR("data_client did not stop");
			 }
		       });
    io.run(); // enter the io_service loop: process both the server and
	    // the client scheduled actions until they stop (empty
	    // action queue)

  const std::vector<packet> &orig = ds.source().original;
  const std::vector<packet> &recv = dc.sink().received;

  // verify number of packets
  BOOST_CHECK_EQUAL(orig.size(), N); // all pkts were extracted
  BOOST_CHECK(recv.size() >= N-K); // at least all blocks except the
				   // last one

  BOOST_CHECK_EQUAL(dc.decoder().blockno(), 9); // synced on the right block
  BOOST_CHECK_GE(dc.decoder().received_count(), K); // got max amount of pkts

  // Check correctness
  std::size_t count_ok = 0;
  std::size_t count_fail = 0;
  auto i = recv.cbegin();
  auto j = orig.cbegin();
  while (i != recv.cend()) {
    BOOST_CHECK(j != orig.cend());
    if (*i) {
      BOOST_CHECK(*i == *j);
      ++count_ok;
    }
    else {
      ++count_fail;
    }
    ++i;;
    ++j;
  }

  BOOST_CHECK_EQUAL(count_fail, dc.decoder().total_failed_count());
  BOOST_CHECK_EQUAL(count_ok, dc.decoder().total_decoded_count());
}


BOOST_AUTO_TEST_CASE(autoclose_client_ack_maxseqno) {
  using namespace std;

  io_service io; // Global io_service object

  const size_t L = 1024; // pkt size
  const size_t K = 100; // block size
  const double c = 0.05;
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
  //ds.target_send_rate(L*2); // Set a target send rate of 2 pkt/s
  ds.enable_ack(true);
  ds.max_sequence_number(175);
  ds.open("127.0.0.1", "9999"); // Setup the data_server socket:
				// random_port -> 127.0.0.1:9999

  dc.setup_decoder(dec_ps); // setup the decoder inside the data_client
  dc.setup_sink(sink_ps); // setup the sink inside the data_client
  dc.enable_ack(true);
  dc.expected_count(N);
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
			 if (!ds.is_stopped()) {
			   ds.stop();
			   BOOST_ERROR("data_server did not stop");
			 }
			 if (!dc.is_stopped()) {
			   dc.stop();
			   BOOST_ERROR("data_client did not stop");
			 }
		       });
    io.run(); // enter the io_service loop: process both the server and
	    // the client scheduled actions until they stop (empty
	    // action queue)

  const std::vector<packet> &orig = ds.source().original;
  const std::vector<packet> &recv = dc.sink().received;

  // verify number of packets
  BOOST_CHECK_EQUAL(orig.size(), N); // all pkts were extracted
  BOOST_CHECK(recv.size() >= N-K); // at least all blocks except the
				   // last one

  BOOST_CHECK_EQUAL(dc.decoder().blockno(), 9); // synced on the right block
  BOOST_CHECK_GE(dc.decoder().received_count(), K); // got max amount of pkts

  // Check correctness
  std::size_t count_ok = 0;
  std::size_t count_fail = 0;
  auto i = recv.cbegin();
  auto j = orig.cbegin();
  while (i != recv.cend()) {
    BOOST_CHECK(j != orig.cend());
    if (*i) {
      BOOST_CHECK(*i == *j);
      ++count_ok;
    }
    else {
      ++count_fail;
    }
    ++i;;
    ++j;
  }

  BOOST_CHECK_EQUAL(count_fail, dc.decoder().total_failed_count());
  BOOST_CHECK_EQUAL(count_ok, dc.decoder().total_decoded_count());
}

BOOST_AUTO_TEST_CASE(client_timeout) {
  using namespace std;

  io_service io; // Global io_service object

  const size_t L = 1024; // pkt size
  const size_t K = 100; // block size
  const double c = 0.05;
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
  //ds.target_send_rate(L*2); // Set a target send rate of 2 pkt/s
  ds.enable_ack(true);
  ds.max_sequence_number(140);
  ds.open("127.0.0.1", "9999"); // Setup the data_server socket:
				// random_port -> 127.0.0.1:9999

  dc.setup_decoder(dec_ps); // setup the decoder inside the data_client
  dc.setup_sink(sink_ps); // setup the sink inside the data_client
  dc.enable_ack(false);
  dc.expected_count(N + K);
  dc.timeout(3);
  dc.bind("9999"); // bind the data_client to the port 9999

  auto srv_ep = ds.server_endpoint(); // get the random source port
				      // used by the data_server
  dc.start_receive(srv_ep); // schdule the data_client to listen on
			    // its bound port for packets from the
			    // server's ip:port

  ds.start(); // start the periodic tx of packets
  // schedule a stop after some time
  boost::asio::steady_timer end_timer(io, std::chrono::seconds(60));
  end_timer.async_wait([&ds,&dc]
		       (const boost::system::error_code&) -> void {
			 if (!ds.is_stopped()) {
			   ds.stop();
			   BOOST_ERROR("data_server did not stop");
			 }
			 if (!dc.is_stopped()) {
			   dc.stop();
			   BOOST_ERROR("data_client did not stop");
			 }
		       });
    io.run(); // enter the io_service loop: process both the server and
	    // the client scheduled actions until they stop (empty
	    // action queue)

  const std::vector<packet> &orig = ds.source().original;
  const std::vector<packet> &recv = dc.sink().received;

  // verify number of packets
  BOOST_CHECK_EQUAL(orig.size(), N); // all pkts were extracted
  BOOST_CHECK_EQUAL(recv.size(), N+K); // all pkts were flushed

  BOOST_CHECK_EQUAL(dc.decoder().blockno(), 10+1); // synced on the right block
  BOOST_CHECK_EQUAL(dc.decoder().received_count(), 0); // past the last block

  // Flushed count
  BOOST_CHECK_EQUAL(dc.decoder().total_decoded_count() +
		    dc.decoder().total_failed_count(), N+K);

  // Check correctness
  std::size_t count_ok = 0;
  std::size_t count_fail = 0;
  auto i = recv.cbegin();
  auto j = orig.cbegin();
  while (i != recv.cend() && j != orig.cend()) {
    if (*i) {
      BOOST_CHECK(*i == *j);
      ++count_ok;
    }
    else {
      ++count_fail;
    }
    ++i;;
    ++j;
  }

  count_fail += recv.size() - orig.size();

  BOOST_CHECK_EQUAL(count_fail, dc.decoder().total_failed_count());
  BOOST_CHECK_EQUAL(count_ok, dc.decoder().total_decoded_count());
}

BOOST_AUTO_TEST_CASE(uep_recv_all) {
  using namespace std;

  io_service io; // Global io_service object

  const size_t L = 1000; // pkt size
  const vector<size_t> Ks = {16,32,64}; // block size
  const vector<size_t> RFs = {4,2,1};
  const size_t EF = 2;
  //const size_t K = 64*3*2; // Total blocksize after expansion
  const size_t Kuep = 64+32+16; // Total blocksize before expansion
  const double c = 0.01;
  const double delta = 0.5;
  const size_t nblocks = 10; // blocks to send
  const mt19937::result_type src_seed = 0x42;

  uep_encoder<std::mt19937>::parameter_set enc_ps{0,0,EF,Ks,RFs,c,delta};
  uep_decoder::parameter_set dec_ps = enc_ps;
  random_packet_source::parameter_set src_ps{src_seed,L,nblocks*Kuep,Ks};
  memory_sink::parameter_set sink_ps;

  data_server<uep_encoder<std::mt19937>,random_packet_source> ds(io);
  data_client<uep_decoder,memory_sink> dc(io);

  ds.setup_encoder(enc_ps); // setup the encoder inside the data_server
  ds.setup_source(src_ps); // setup the source  inside the data_server
  //ds.target_send_rate(L*5); // Set a target send rate of 5 pkt/s
  ds.enable_ack(true);
  //ds.max_sequence_number(140);
  ds.open("127.0.0.1", "9999"); // Setup the data_server socket:
				// random_port -> 127.0.0.1:9999

  dc.setup_decoder(dec_ps); // setup the decoder inside the data_client
  dc.setup_sink(sink_ps); // setup the sink inside the data_client
  dc.enable_ack(true);
  dc.expected_count(nblocks*Kuep);
  dc.timeout(1);
  dc.bind("9999"); // bind the data_client to the port 9999

  auto srv_ep = ds.server_endpoint(); // get the random source port
				      // used by the data_server
  dc.start_receive(srv_ep); // schdule the data_client to listen on
			    // its bound port for packets from the
			    // server's ip:port

  ds.start(); // start the periodic tx of packets
  // schedule a stop after some time
  boost::asio::steady_timer end_timer(io, std::chrono::seconds(60));
  end_timer.async_wait([&ds] (const boost::system::error_code &ec) -> void {
      if (ec == boost::asio::error::operation_aborted)
	return;
      if (!ds.is_stopped()) {
	ds.stop();
	BOOST_ERROR("data_server did not stop");
      }
    });
  end_timer.async_wait([&dc] (const boost::system::error_code &ec) -> void {
      if (ec == boost::asio::error::operation_aborted)
	return;
      if (!dc.is_stopped()) {
	dc.stop();
	BOOST_ERROR("data_client did not stop");
      }
    });
  io.run(); // enter the io_service loop: process both the server and
	    // the client scheduled actions until they stop (empty
	    // action queue)

  const std::vector<fountain_packet> &orig = ds.source().original_fp;
  const std::vector<fountain_packet> &recv = dc.sink().received_fp;

  // verify number of packets
  BOOST_CHECK_EQUAL(orig.size(), nblocks*Kuep); // all pkts were extracted
  BOOST_CHECK_EQUAL(recv.size(), nblocks*Kuep); // all pkts were flushed

  BOOST_CHECK_EQUAL(dc.decoder().blockno(), nblocks-1); // synced on the last block
  BOOST_CHECK_GE(dc.decoder().received_count(), Kuep); // min to decode fully

  // Flushed count
  BOOST_CHECK_EQUAL(dc.decoder().total_decoded_count(), nblocks*Kuep);
  BOOST_CHECK_EQUAL(dc.decoder().total_failed_count(), 0);

  // Check correctness
  std::size_t count_ok = 0;
  std::size_t count_fail = 0;
  auto i = recv.cbegin();
  auto j = orig.cbegin();
  while (i != recv.cend() && j != orig.cend()) {
    if (*i) {
      BOOST_CHECK(i->buffer() == j->buffer());
      BOOST_CHECK(i->getPriority() == j->getPriority());
      ++count_ok;
    }
    else {
      ++count_fail;
    }
    ++i;
    ++j;
  }

  count_fail += recv.size() - orig.size();

  BOOST_CHECK_EQUAL(count_fail, dc.decoder().total_failed_count());
  BOOST_CHECK_EQUAL(count_ok, dc.decoder().total_decoded_count());
}
