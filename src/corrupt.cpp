#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>
#include <array>

#include "svc_segmenter.hpp"

#include "H264AVCCommonLib.h"
#include "H264AVCDecoderLib.h"
#include "CreaterH264AVCDecoder.h"
#include "H264AVCVideoIoLib.h"
#include "CreaterH264AVCDecoder.h"
#include "ReadBitstreamFile.h"
#include "WriteBitstreamToFile.h"

using namespace std;
using namespace uep;

constexpr double pseg(double pbit, size_t len) {
  return 1-pow(1-pbit, 8*len);
}

int main(int argc, char **argv) {
  if (argc != 4) {
    cerr << "Give 3 args: in_file out_file pbit" << endl;
    return 1;
  }

  double pbit = atof(argv[3]);

  svc_segmenter segm(argv[1]);
  svc_assembler asmb(argv[2]);

  mt19937 rng;
  bernoulli_distribution segerr;
  
  size_t in = 0;
  size_t out = 0;
  
  while (segm) {
    auto s = segm.next_segment();
    ++in;
    cerr << "Segment of length " << s.size()
	 << " and priority " << (int)s.getPriority() << endl;

    double ps = pseg(pbit,s.size());
    
    if (segerr(rng, decltype(segerr)::param_type(ps))) {
      if (s.getPriority() > 0) {
	cerr << "Drop segment" << endl;
	continue;
      }
      else
	cerr << "Don't drop high priority segment" << endl;
    }

    if (s.size() == 1924) {
      cerr << "Drop 1924" << endl;
      continue;
    }
    
    asmb.push_segment(s.buffer());
    ++out;
  }

  cerr << "Read " << in << " segments"
       << " written " << out << " segments"
       << endl;
  
  return 0;
}
