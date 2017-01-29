#ifndef RNG_HPP
#define RNG_HPP

#include <algorithm>
#include <cstdint>
#include <functional>
#include <random>
#include <vector>

/** Implement a discrete distribution with elements in [1,K] according
 *  to a specified PMD.
 */
class degree_distribution {
public:
  /** Type of the PMD function. */
  typedef std::function<double(std::uint_fast32_t)> pmd_t;

  /** Use the PMD to build the discrete distribution in [1,K]. */
  explicit degree_distribution(std::uint_fast32_t K, const pmd_t &pmd);
  /** Correct destruction when used polymorphically. */
  virtual ~degree_distribution() = default;
  /** The maximum degree. */
  std::uint_fast32_t K() const;
  /** The PMD function. */
  pmd_t pmd() const;
  /** Generate a degree using the RNG g. */
  template<class Gen> std::uint_fast32_t operator()(Gen &g);

private:
  const std::uint_fast32_t K_;
  const pmd_t pmd_;
  std::discrete_distribution<std::uint_fast32_t> distrib;
};

/** Produces soliton-distributed random numbers. */
class soliton_distribution : public degree_distribution {
public:
  /** Returns the soliton probability mass distribution for a certain
   *  blocksize K and degree d.
   */
  static double soliton_pmd(std::uint_fast32_t K, std::uint_fast32_t d);

  explicit soliton_distribution(std::uint_fast32_t input_pkt_count);
};

/** Produces robust-soliton-distributed random numbers. */
class robust_soliton_distribution : public degree_distribution {
public:
  static double S(std::uint_fast32_t K,
		  double c,
		  double delta);
  static double tau(std::uint_fast32_t K_S,
		    double S_delta,
		    std::uint_fast32_t i);
  static double beta(std::uint_fast32_t K,
		     std::uint_fast32_t K_S,
		     double S_delta);
  static double robust_pmd(std::uint_fast32_t K,
			   double c,
			   double delta,
			   std::uint_fast32_t d);

  explicit robust_soliton_distribution(std::uint_fast32_t input_pkt_count,
				       double c,
				       double delta);
  /** Return the c coefficient */
  double c() const;
  /** Return the delta_max parameter */
  double delta() const;

private:
  const double c_;
  const double delta_;
};

/** Chooses at random which input packets to mix into the next coded
 *  packet.
 */
class fountain {
public:
  typedef std::mt19937 generator_type;
  typedef std::uint_fast32_t position_type;
  typedef std::vector<position_type> row_type;

  /** Build a fountain using the specified degree distribution */
  explicit fountain(const degree_distribution &deg);
  /** Build a new fountain using the specified degree distribution and
   *  RNG seed.
   */
  explicit fountain(const degree_distribution &deg,
		    generator_type::result_type seed);
  /** Generate the next packet selection. */
  row_type next_row();
  /** Return the number of packet selections already generated. */
  typename row_type::size_type generated_rows() const;
  /** Return the input blocksize */
  std::uint_fast32_t K() const;
  /** Reset to the initial state using the default seed */
  void reset();
  /** Reset to the initial state using the specified seed */
  void reset(generator_type::result_type seed);

private:
  generator_type generator;
  degree_distribution degree_distr;
  std::uniform_int_distribution<std::uint_fast32_t> packet_distr;
  typename row_type::size_type sel_count;
};

// Template definitions

template<class Gen>
std::uint_fast32_t degree_distribution::operator()(Gen &g) {
  return distrib(g) + 1;
}

#endif
