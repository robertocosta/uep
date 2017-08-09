extern "C" {
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
}

#include "encoder.hpp"
#include "decoder.hpp"

using namespace std;
using namespace uep;

packet random_pkt(size_t size) {
  static std::independent_bits_engine<std::mt19937, CHAR_BIT, unsigned char> g;
  packet p;
  p.resize(size);
  for (size_t i=0; i < size; i++) {
    p[i] = g();
  }
  return p;
}

void run_enc_dec(size_t K, double c, double delta,
		 size_t nblocks, size_t L) {
  log::default_logger perf_lg(boost::log::keywords::channel = log::performance);

  BOOST_LOG(perf_lg) << "run_enc_dec K=" << K
		     << " c=" << c
		     << " delta=" << delta
		     << " nblocks=" << nblocks
		     << " L=" << L;

  lt_encoder<mt19937> enc(K,c,delta);
  lt_decoder dec(K,c,delta);

  // Load the encoder
  for (size_t i = 0; i < nblocks*K; ++i) {
    enc.push(random_pkt(L));
  }

  // Decode all the packets
  while (enc.size() > 0) {
    dec.push(enc.next_coded());
    while (!dec.has_decoded()) {
      dec.push(enc.next_coded());
    }
    enc.next_block();
  }
}

int main(int,char**) {
  pid_t pid, pid2;
  int status, status2;
  pid = fork();
  if (pid == 0) {
    log::init("mp_plots_A.log");
    log::default_logger perf_lg(boost::log::keywords::channel = log::performance);
    BOOST_LOG(perf_lg) << "RUN CASE A";
    run_enc_dec(10000, 0.01, 0.5, 3, 1);
    BOOST_LOG(perf_lg) << "DONE A";
  }
  else if (pid < 0) {
    throw std::runtime_error("Fork failed");
  }
  else {
    pid2 = fork();
    if (pid2 == 0) {
      log::init("mp_plots_B.log");
      log::default_logger perf_lg(boost::log::keywords::channel = log::performance);
      BOOST_LOG(perf_lg) << "RUN CASE B";
      run_enc_dec(10000, 0.03, 0.5, 3, 1);
      BOOST_LOG(perf_lg) << "DONE B";
    }
    else if (pid < 0) {
      throw std::runtime_error("Fork failed");
    }
    else {
      log::init("mp_plots_C.log");
      log::default_logger perf_lg(boost::log::keywords::channel = log::performance);
      BOOST_LOG(perf_lg) << "RUN CASE C";
      run_enc_dec(10000, 0.1, 0.5, 3, 1);
      BOOST_LOG(perf_lg) << "DONE C";
      wait(&status);
      wait(&status2);

    }
  }
  return 0;
}
