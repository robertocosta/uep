#include <cstdlib>

extern "C" {
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
}

#include "cmake_defines.hpp"
#include "decoder.hpp"
#include "encoder.hpp"

using namespace std;
using namespace uep;

template <class Func>
pid_t run_forked(const Func &f) {
  pid_t pid = fork();
  if (pid < 0) {
    throw std::runtime_error("Fork failed");
  }
  else if (pid == 0) {
    f();
    exit(EXIT_SUCCESS);
  }
  else {
    return pid;
  }
}

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
  std::vector<pid_t> children_pids;
  pid_t pid;

  pid = run_forked([]{
      log::init("mp_plots_A.log");
      log::default_logger perf_lg(boost::log::keywords::channel = log::performance);
      BOOST_LOG(perf_lg) << "RUN CASE A";
      BOOST_LOG(perf_lg) << "Git commit: " << GIT_COMMIT_SHA1;
      run_enc_dec(10000, 0.01, 0.5, 3, 1024);
      BOOST_LOG(perf_lg) << "DONE A";
    });
  children_pids.push_back(pid);
  pid = run_forked([]{
      log::init("mp_plots_B.log");
      log::default_logger perf_lg(boost::log::keywords::channel = log::performance);
      BOOST_LOG(perf_lg) << "RUN CASE B";
      BOOST_LOG(perf_lg) << "Git commit: " << GIT_COMMIT_SHA1;
      run_enc_dec(10000, 0.03, 0.5, 3, 1024);
      BOOST_LOG(perf_lg) << "DONE B";
    });
  children_pids.push_back(pid);
  pid = run_forked([]{
      log::init("mp_plots_C.log");
      log::default_logger perf_lg(boost::log::keywords::channel = log::performance);
      BOOST_LOG(perf_lg) << "RUN CASE C";
      BOOST_LOG(perf_lg) << "Git commit: " << GIT_COMMIT_SHA1;
      run_enc_dec(10000, 0.1, 0.5, 3, 1024);
      BOOST_LOG(perf_lg) << "DONE C";
    });
  children_pids.push_back(pid);

  size_t nprocs = children_pids.size();
  for (size_t i = 0; i < nprocs; ++i) {
    int rc;
    pid_t pid_done = wait(&rc);
    if (rc == EXIT_SUCCESS) {
      auto newend = remove(children_pids.begin(), children_pids.end(), pid_done);
      children_pids.erase(newend, children_pids.end());
    }
    else {
      throw runtime_error("A child failed");
    }
  }

  if (!children_pids.empty())
    throw runtime_error("Some child did not exit");

  return 0;
}
