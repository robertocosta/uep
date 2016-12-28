#include <algorithm>
#include <random>
#include <vector>

/** Produces soliton-distributed random numbers. */
template<class IntType = int>
class soliton_distribution {
public:
  soliton_distribution(IntType input_pkt_count);
  /** Generate a new number using the generator g. */
  template<class Generator> int operator()(Generator &g);
  /** Return the input block size. */
  IntType K() const;

private:
  const IntType K_;
  std::discrete_distribution<IntType> distrib;

  /** Returns the soliton probability mass distribution for a certain
   *  blocksize K and degree d.
   */
  static double soliton_pmd(IntType K, IntType d);
};

/** Chooses at random which input packets to mix into the next coded
 *  packet.
 */
template<class IntType = int>
class fountain {
public:
  typedef std::mt19937 generator_type;
  typedef std::vector<IntType> selection_type;

  /** Build a new fountain using the default seed. */
  fountain(IntType input_pkt_count);
  /** Build a new fountain using the specified seed. */
  fountain(IntType input_pkt_count, generator_type::result_type seed);
  /** Generate the next packet selection. */
  selection_type next_packet_selection();
  /** Return the number of packet selections already generated. */
  typename selection_type::size_type generated_selections() const;
  /** Return the input blocksize */
  IntType K() const;
  /** Reset to the initial state using the default seed */
  void reset();
  /** Reset to the initial state using the specified seed */
  void reset(generator_type::result_type seed);

private:
  const IntType K_;
  std::mt19937 generator;
  soliton_distribution<IntType> degree_distr;
  std::uniform_int_distribution<IntType> packet_distr;
  typename selection_type::size_type sel_count;
};

template <class IntType>
soliton_distribution<IntType>::soliton_distribution(IntType input_pkt_count) :
  K_(input_pkt_count) {
  std::vector<double> weights;
  for (IntType d = 1; d <= K_; d++) {
    weights.push_back(soliton_pmd(K_, d));
  }

  typedef typename decltype(distrib)::param_type p_type;
  p_type p(weights.begin(), weights.end());
  distrib.param(p);
}

template<class IntType>
template<class Generator>
int soliton_distribution<IntType>::operator()(Generator &g) {
  return distrib(g)+1;
}

template<class IntType>
IntType soliton_distribution<IntType>::K() const {
  return K_;
}

template<class IntType>
double soliton_distribution<IntType>::soliton_pmd(IntType K, IntType d) {
  if (d == 1) return 1/(double)K;
  else if (d >= 2 && d <= K) return 1/((double)d*(d-1));
  else return 0;
}

template<class IntType>
fountain<IntType>::fountain(IntType input_pkt_count) :
  K_(input_pkt_count),
  degree_distr(K_),
  packet_distr(1, K_) {
}

template<class IntType>
fountain<IntType>::fountain(IntType input_pkt_count,
			    generator_type::result_type seed) :
  fountain(input_pkt_count) {
  generator.seed(seed);
}

template<class IntType>
typename fountain<IntType>::selection_type
fountain<IntType>::next_packet_selection() {
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

template<class IntType>
typename fountain<IntType>::selection_type::size_type
fountain<IntType>::generated_selections() const {
  return sel_count;
}

template<class IntType>
IntType fountain<IntType>::K() const {
  return K_;
}

template<class IntType>
void fountain<IntType>::reset() {
  generator.seed();
  sel_count = 0;
}

template<class IntType>
void fountain<IntType>::reset(generator_type::result_type seed) {
  generator.seed(seed);
  sel_count = 0;
}
