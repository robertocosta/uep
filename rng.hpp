#include <algorithm>
#include <random>
#include <vector>

template<class IntType = int>
class soliton_distribution {
public:
  soliton_distribution(IntType input_pkt_count) :
    K_(input_pkt_count) {
    std::vector<double> weights;
    for (IntType d = 1; d <= K_; d++) {
      weights.push_back(soliton_pmd(K_, d));
    }
    
    typedef typename decltype(distrib)::param_type p_type;
    p_type p(weights.begin(), weights.end());
    distrib.param(p);
  }

  template<class Generator>
  int operator()(Generator &g) {
    return distrib(g)+1;
  }

  IntType K() const { return K_; }

private:
  const IntType K_;
  std::discrete_distribution<IntType> distrib;

  static double soliton_pmd(IntType K, IntType d) {
    if (d == 1) return 1/(double)K;
    else if (d >= 2 && d <= K) return 1/((double)d*(d-1));
    else return 0;
  }
};

template <class IntType = int>
class fountain {
public:
  typedef std::mt19937 generator_type;
  typedef std::vector<IntType> selection_type;

  fountain(IntType input_pkt_count) : 
    K_(input_pkt_count),
    degree_distr(K_),
    packet_distr(1, K_) {
  }
  
  fountain(IntType input_pkt_count,
	   generator_type::result_type seed) :
    fountain(input_pkt_count) {
    generator.seed(seed);
  }

  selection_type next_packet_selection() {
    IntType degree = degree_distr(generator);
    selection_type s(degree);
    for (int i = 0; i < degree; i++) {
      IntType si;
      do {
	si = packet_distr(generator);
      }	while (std::find(s.begin(), s.end(), si) != s.end());
      s[i] = si;
    }
    sel_count++;
    return s;
  }

  typename selection_type::size_type generated_selections() const {
    return sel_count;
  }

  IntType K() const {
    return K_;
  }

  void reset() {
    generator.seed();
    sel_count = 0;
  }

  void reset(generator_type::result_type seed) {
    generator.seed(seed);
    sel_count = 0;
  }
  
private:
  const IntType K_;
  std::mt19937 generator;
  soliton_distribution<IntType> degree_distr;
  std::uniform_int_distribution<IntType> packet_distr;
  typename selection_type::size_type sel_count;
};
