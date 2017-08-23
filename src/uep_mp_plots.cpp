#include <cstdlib>

extern "C" {
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
}

#include "cmake_defines.hpp"
#include "uep_decoder.hpp"
#include "uep_encoder.hpp"

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

fountain_packet random_pkt(size_t size, size_t priority) {
  static std::independent_bits_engine<std::mt19937, CHAR_BIT, unsigned char> g;
  fountain_packet p;
  p.resize(size);
  for (size_t i=0; i < size; i++) {
    p[i] = g();
  }
  p.setPriority(priority);
  return p;
}

void run_enc_dec(initializer_list<size_t> Ks,
		 initializer_list<size_t> RFs,
		 size_t EF,
		 double c, double delta,
		 size_t nblocks, size_t L, size_t max_size) {
  log::default_logger perf_lg(boost::log::keywords::channel = log::performance);

  lt_uep_parameter_set ps{0,0,EF,Ks,RFs,c,delta};

  BOOST_LOG(perf_lg) << "run_enc_dec Ks=" << ps.Ks
		     << " RFs=" << ps.RFs
		     << " EF=" << EF
		     << " c=" << c
		     << " delta=" << delta
		     << " nblocks=" << nblocks
		     << " L=" << L
		     << " max_size=" << max_size;
  
  uep_encoder<mt19937> enc(ps);
  uep_decoder dec(ps);

  // Load the encoder
  for (size_t i = 0; i < nblocks; ++i) {
    auto K_i = Ks.begin();
    for (size_t j = 0; j < Ks.size(); ++j) {
      size_t K = *K_i++;
      for (size_t l = 0; l < K; ++l) {
	enc.push(random_pkt(L, j));
      }
    }
  }

  // Decode all the packets
  std::list<fountain_packet> coded;
  while (enc.size() > 0) {
    do {
      while (coded.size() < max_size) {
	coded.push_back(enc.next_coded());
      }
      dec.push(make_move_iterator(coded.begin()),
	       make_move_iterator(coded.end()));
      coded.clear();
    } while (!dec.has_decoded());
    enc.next_block();
  }
}

int main(int,char**) {
  std::vector<pid_t> children_pids;
  pid_t pid;
  constexpr size_t L = 1500 - 40 - 8;
  constexpr size_t nblocks = 3;
  constexpr size_t max_size = 1;

  pid = run_forked([]{
      log::init("uep_mp_plots_A.log");
      log::default_logger perf_lg(boost::log::keywords::channel = log::performance);
      BOOST_LOG(perf_lg) << "RUN CASE A";
      BOOST_LOG(perf_lg) << "Git commit: " << GIT_COMMIT_SHA1;
      run_enc_dec({100,300}, {2,1}, 2, 0.01, 0.5, nblocks, L, max_size);
      BOOST_LOG(perf_lg) << "DONE A";
    });
  children_pids.push_back(pid);
  pid = run_forked([]{
      log::init("uep_mp_plots_B.log");
      log::default_logger perf_lg(boost::log::keywords::channel = log::performance);
      BOOST_LOG(perf_lg) << "RUN CASE B";
      BOOST_LOG(perf_lg) << "Git commit: " << GIT_COMMIT_SHA1;
      run_enc_dec({100,300}, {2,1}, 2, 0.03, 0.5, nblocks, L, max_size);
      BOOST_LOG(perf_lg) << "DONE B";
    });
  children_pids.push_back(pid);
  pid = run_forked([]{
      log::init("uep_mp_plots_C.log");
      log::default_logger perf_lg(boost::log::keywords::channel = log::performance);
      BOOST_LOG(perf_lg) << "RUN CASE C";
      BOOST_LOG(perf_lg) << "Git commit: " << GIT_COMMIT_SHA1;
      run_enc_dec({100,300}, {2,1}, 2, 0.1, 0.5, nblocks, L, max_size);
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
