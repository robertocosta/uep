#include <iostream>

#include <unistd.h>

#include "log.hpp"
#include "nal_reader.hpp"

int main(int argc, char **argv) {
#ifdef UEP_VERBOSE_LOGS
  uep::log::init("nal_overhead.log");
#else
  uep::log::init("nal_overhead.log", uep::log::info);
#endif
  uep::log::default_logger basic_lg = uep::log::basic_lg::get();
  uep::log::default_logger perf_lg = uep::log::perf_lg::get();

  std::string streamname;
  std::size_t pktsize;

  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, "n:L:")) != -1) {
    switch (c) {
    case 'n':
      streamname = optarg;
      break;
    case 'L':
      pktsize = std::strtoull(optarg, nullptr, 10);
      break;
    default:
      std::cerr << "Usage: " << argv[0]
		<< " -n <streamname>"
		<< " -L <pktsize>"
		<< std::endl;
      return 2;
    }
  }

  uep::nal_reader nl(streamname, pktsize);
  std::size_t npkts0 = 0, npkts1 = 0;
  // Read all packets to count the sizes + padding
  while (nl.has_packet()) {
    fountain_packet p = nl.next_packet();
    if (p.getPriority() == 0)
      ++npkts0;
    else if (p.getPriority() == 1)
      ++npkts1;
    else
      throw std::runtime_error("prio not 0 or 1");
  }

  std::size_t oh0 = nl.total_overhead().at(0);
  std::size_t oh1 = nl.total_overhead().at(1);
  std::size_t size0 = nl.total_read().at(0);
  std::size_t size1 = nl.total_read().at(1);

  std::cout << "npkts0=" << npkts0 << std::endl;
  std::cout << "npkts1=" << npkts1 << std::endl;
  std::cout << "size0=" << size0 << std::endl;
  std::cout << "size1=" << size1 << std::endl;
  std::cout << "oh0=" << oh0 << std::endl;
  std::cout << "oh1=" << oh1 << std::endl;
}
