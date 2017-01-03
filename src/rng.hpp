#ifndef RNG_HPP
#define RNG_HPP

#include <cstdint>
#include <random>
#include <vector>

/** Produces soliton-distributed random numbers. */
class soliton_distribution {
public:
  explicit soliton_distribution(std::uint_fast32_t input_pkt_count);
  /** Generate a new number using the generator g. */
  template<class Generator>
  std::uint_fast32_t operator()(Generator &g) {
    return distrib(g)+1;
  }
  /** Return the input block size. */
  std::uint_fast32_t K() const;

private:
  const std::uint_fast32_t K_;
  std::discrete_distribution<std::uint_fast32_t> distrib;

  /** Returns the soliton probability mass distribution for a certain
   *  blocksize K and degree d.
   */
  static double soliton_pmd(std::uint_fast32_t K, std::uint_fast32_t d);
};

/** Chooses at random which input packets to mix into the next coded
 *  packet.
 */
class fountain {
public:
  typedef std::mt19937 generator_type;
  typedef std::vector<std::uint_fast32_t> row_type;

  /** Build a new fountain using the default seed. */
  explicit fountain(std::uint_fast32_t input_pkt_count);
  /** Build a new fountain using the specified seed. */
  explicit fountain(std::uint_fast32_t input_pkt_count,
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
  const std::uint_fast32_t K_;
  generator_type generator;
  soliton_distribution degree_distr;
  std::uniform_int_distribution<std::uint_fast32_t> packet_distr;
  typename row_type::size_type sel_count;
};

#endif
