#ifndef UEP_MP_MESSAGE_PASSING_HPP
#define UEP_MP_MESSAGE_PASSING_HPP

#include <algorithm>
#include <chrono>
#include <memory>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/iterator/transform_iterator.hpp>

#include "counter.hpp"
#include "skip_false_iterator.hpp"

namespace uep { namespace mp {

/** Type traits that provide the interface for the symbols used in the
 *  mp_context class.
 */
template <class Symbol>
class symbol_traits {
private:
  static void swap_syms(Symbol &lhs, Symbol &rhs) {
    using std::swap;
    swap(lhs, rhs);
  }

  static_assert(std::is_move_constructible<Symbol>::value,
		"Symbol must be move-constructible");
  static_assert(std::is_move_assignable<Symbol>::value,
		"Symbol must be move-assignable");

public:
  /** Create an empty symbol. */
  static Symbol create_empty() {
    return Symbol();
  }

  /** True when a symbol is empty. */
  static bool is_empty(const Symbol &s) {
    return !s;
  }

  /** Perform the in-place XOR between two symbols. */
  static void inplace_xor(Symbol &lhs, const Symbol &rhs) {
    lhs ^= rhs;
  }

  /** Swap two symbols. */
  static void swap(Symbol &lhs, Symbol &rhs) {
    // Avoid using the function with the same name
    swap_syms(lhs,rhs);
  }
};

}}

// Backward compatibility
namespace uep { namespace utils {
using mp::symbol_traits;
}}

namespace uep { namespace mp {

/** Class used to execute the message-passing algorithm.
 *
 *  The context controls a bipartite graph, which is initialized with
 *  a fixed number of empty input symbols and is incrementally built
 *  from each output symbol, together with its edges. Parallel edges
 *  are allowed and are handled like all the others.
 *
 *  The algorithm can be run many times without losing the previously
 *  decoded packets.
 *
 *  The generic symbol type `Symbol` is manipulated through the traits
 *  class `SymbolTraits`.
 */
template <class Symbol, class SymbolTraits = symbol_traits<Symbol>>
class mp_context {
private:
  /** Struct used to carry additional information with the symbols. */
  struct node;
  /** Converter from node pointers to a const reference to their symbol. */
  struct node2sym;
public:
  /** The type used to represent the symbols. */
  typedef Symbol symbol_type;
  /** The type used for the symbol traits. */
  typedef SymbolTraits symbol_traits;
  /** Constant iterator over the input symbols. */
  typedef boost::transform_iterator<
    node2sym,
    typename std::vector<std::unique_ptr<node>>::const_iterator
    > inputs_iterator;
  /** Constant iterator that skips false (empty) symbols. */
  typedef utils::skip_false_iterator<inputs_iterator> decoded_iterator;

  /** Construct a context with in_size default-constructed input symbols. */
  explicit mp_context(std::size_t in_size);

  /** Copy constructor. */
  mp_context(const mp_context &other);
  /** Move constructor. */
  mp_context(mp_context &&other);

  /** Copy-assignment operator. */
  mp_context &operator=(const mp_context &other);
  /** Move-assignment operator. */
  mp_context &operator=(mp_context &&other);

  /** Auto-generated destructor. */
  ~mp_context() = default;

  /** Add an output symbol to the bipartite graph, connected with the
   *  input symbols given by the iterator pair. If the input symbols
   *  are partially decoded, the new output symbol is XOR-ed with
   *  those linked to it.
   */
  template <class EdgeIter>
  void add_output(symbol_type &&s, EdgeIter edges_begin, EdgeIter edges_end);

  /** Call add_output(symbol_type&&,EdgeIter,EdgeIter) with a copy of
   *  the given symbol.
   */
  template <class EdgeIter>
  void add_output(const symbol_type &s, EdgeIter edges_begin, EdgeIter edges_end);

  /** Run the message-passing algorithm with the current context.
   *  After the input symbols are fully decoded, this method does
   *  nothing.  If the input symbols are partially decoded, they are
   *  still available via decoded_begin(), decoded_end() and
   *  successive invocations of run() try to decode more symbols.
   */
  void run();

  /** Reset the context to the initial state. */
  void reset();

  /** Return the number of input symbols. */
  std::size_t input_size() const;
  /** Return the number of output symbols. */
  std::size_t output_size() const;
  /** Return the number of input symbols that have been correctly
   *  decoded up to the last call to run().
   */
  std::size_t decoded_count() const;
  /** True when all the input symbols have been decoded. */
  bool has_decoded() const;
  /** Return the time it took to complete the last call to run. */
  double run_duration() const;
  /** Return the average ripple size since the last reset. */
  double average_ripple_size() const;

  /** Return an iterator to the start of the set of input symbols.
   *  The symbols are either decoded or convertible to false.
   */
  inputs_iterator input_symbols_begin() const;
  /** Return an iterator to the end of the set of input symbols.
   *  The symbols are either decoded or convertible to false.
   */
  inputs_iterator input_symbols_end() const;

  /** Return an iterator to the start of the set of decoded
   *  symbols. This iterator gives access to the same symbols as
   *  input_symbols_begin, skipping the ones that convert to false.
   */
  decoded_iterator decoded_symbols_begin() const;

  /** Return an iterator to the end of the set of decoded symbols. */
  decoded_iterator decoded_symbols_end() const;

private:
  /** Pointers to the input nodes. These manage the lifetime of the
   *  nodes.
   */
  std::vector<std::unique_ptr<node>> inputs;
  /** Pointers to the output nodes. These manage the lifetime of the
   *  nodes.
   */
  std::vector<std::unique_ptr<node>> outputs;
  node *ripple_first; /**< Start of the singly linked list of nodes in
		       *   the ripple.
		       */
  node *degone_first; /**< Start of the linked list of nodes with
		       *   degree one.
		       */
  node *degone_last; /**< End of the linked list of nodes with
		      *   degree one.
		      */
  std::size_t decoded_count_; /**< Number of currenlty decoded
			       *   packets.
			       */

  std::chrono::duration<double> last_run_time; /**< The time that the
						*   last call to run
						*   took.
						*/

  stat::average_counter _ripple_size; /**< Mesaure the average ripple
				       *   size since the last reset.
				       */

  /** Insert an output node in the list of degree one nodes. */
  void insert_degone(node *np);
  /** Remove an output node from the list of degree one nodes. */
  void remove_degone(node *np);
  /** Insert an input node in the ripple. */
  void insert_ripple(node *np);

  /** Decode symbols with output degree one and update the ripple and
   *  degone lists.
   */
  void decode_degree_one();
  /** Process the input symbols in the ripple to remove some edges by
   *  XOR-ing with the decoded values.
   */
  void process_ripple();

public: // old typedefs
  typedef inputs_iterator input_symbols_iterator;
  typedef decoded_iterator decoded_symbols_iterator;
};

template <class Symbol, class SymbolTraits>
struct mp_context<Symbol,SymbolTraits>::node {
  symbol_type symbol; /**< The symbol carried by this node. */
  std::size_t position; /**< The position of this node in the inputs
			 *   or outputs list.
			 */
  node *next; /**< Pointer to the next node in a singly linked
	       *   list. Can be used either by the ripple or the
	       *   degone lists.
	       */
  node *prev; /**< Pointer to the previous node in a linked list. Is
	       *   used by the degone list.
	       */
  std::unordered_multiset<std::size_t> edges; /**< Indices of the nodes
					       *   connected to this.
					       */
};

template <class Symbol, class SymbolTraits>
struct mp_context<Symbol,SymbolTraits>::node2sym {
  const symbol_type &operator()(const std::unique_ptr<node> &np) const {
    return np->symbol;
  }
};

//// mp_context<Symbol,SymbolTraits> template definitions ////

template <class Symbol, class SymbolTraits>
mp_context<Symbol,SymbolTraits>::mp_context(std::size_t in_size) :
  ripple_first(nullptr),
  degone_first(nullptr),
  degone_last(nullptr),
  decoded_count_(0),
  last_run_time(std::chrono::duration<double>::zero()) {
  // Build in_size empty input nodes
  inputs.resize(in_size);
  std::size_t pos = 0;
  for (std::unique_ptr<node> &np : inputs) {
    np = std::make_unique<node>();
    np->symbol = symbol_traits::create_empty();
    np->position = pos++;
  }
}

template <class Symbol, class SymbolTraits>
mp_context<Symbol,SymbolTraits>::mp_context(mp_context &&other) :
  inputs(std::move(other.inputs)),
  outputs(std::move(other.outputs)),
  ripple_first(std::move(other.ripple_first)),
  degone_first(std::move(other.degone_first)),
  degone_last(std::move(other.degone_last)),
  decoded_count_(std::move(other.decoded_count_)),
  last_run_time(std::move(other.last_run_time)),
  _ripple_size(std::move(other._ripple_size)) {
}

template <class Symbol, class SymbolTraits>
mp_context<Symbol,SymbolTraits>::mp_context(const mp_context &other) :
  inputs(other.inputs.size()), // copied after
  outputs(other.outputs.size()), // copied after
  ripple_first(nullptr), // Is always null outside of run()
  degone_first(nullptr), // The list must be rebuilt
  degone_last(nullptr), // The list must be rebuilt
  decoded_count_(other.decoded_count_),
  last_run_time(other.last_run_time),
  _ripple_size(other._ripple_size) {
  std::transform(other.inputs.cbegin(), other.inputs.cend(), inputs.begin(),
		 [](const std::unique_ptr<node> &p){
		   return std::make_unique<node>(*p);
		 });
  std::transform(other.outputs.cbegin(), other.outputs.cend(), outputs.begin(),
		 [this](const std::unique_ptr<node> &p){
		   auto p_new = std::make_unique<node>(*p);
		   if (p_new->edges.size() == 1) {
		     insert_degone(p_new.get());
		   }
		   return p_new;
		 });
}

template <class Symbol, class SymbolTraits>
mp_context<Symbol,SymbolTraits> &mp_context<Symbol,SymbolTraits>::operator=(mp_context &&other) {
  inputs = std::move(other.inputs);
  outputs = std::move(other.outputs);
  ripple_first = std::move(other.ripple_first);
  degone_first = std::move(other.degone_first);
  degone_last = std::move(other.degone_last);
  decoded_count_ = std::move(other.decoded_count_);
  last_run_time = std::move(other.last_run_time);
  _ripple_size = std::move(other._ripple_size);
  return *this;
}

template <class Symbol, class SymbolTraits>
mp_context<Symbol,SymbolTraits> &mp_context<Symbol,SymbolTraits>::operator=(const mp_context &other) {
  inputs.resize(other.inputs.size()); // copied after
  outputs.resize(other.outputs.size()); // copied after
  ripple_first = nullptr; // Is always null outside of run()
  degone_first = nullptr; // The list must be rebuilt
  degone_last = nullptr; // The list must be rebuilt
  decoded_count_ = other.decoded_count_;
  last_run_time = other.last_run_time;
  _ripple_size = other._ripple_size;
  std::transform(other.inputs.cbegin(), other.inputs.cend(), inputs.begin(),
		 [](const std::unique_ptr<node> &p){
		   return std::make_unique<node>(*p);
		 });
  std::transform(other.outputs.cbegin(), other.outputs.cend(), outputs.begin(),
		 [this](const std::unique_ptr<node> &p){
		   auto p_new = std::make_unique<node>(*p);
		   if (p_new->edges.size() == 1) {
		     insert_degone(p_new.get());
		   }
		   return p_new;
		 });
  return *this;
}

template <class Symbol, class SymbolTraits>
template <class EdgeIter>
void mp_context<Symbol,SymbolTraits>::add_output(const symbol_type &s,
						 EdgeIter edges_begin, EdgeIter edges_end) {
  symbol_type s_copy(s);
  add_output(std::move(s_copy), edges_begin, edges_end);
}

template <class Symbol, class SymbolTraits>
template <class EdgeIter>
void mp_context<Symbol,SymbolTraits>::add_output(symbol_type &&s,
						 EdgeIter edges_begin, EdgeIter edges_end) {
  // Move the new symbol in an output node
  outputs.push_back(std::make_unique<node>());
  std::unique_ptr<node> &np = outputs.back();
  np->symbol = std::move(s);
  np->position = outputs.size() - 1;
  // Add the edges
  for (EdgeIter i = edges_begin; i != edges_end; ++i) {
    std::unique_ptr<node> &inp = inputs.at(*i);
    if (!symbol_traits::is_empty(inp->symbol)) {
      // input is already decoded, ignore edge and XOR the new output
      symbol_traits::inplace_xor(np->symbol, inp->symbol);
    }
    else {
      np->edges.insert(*i); // Allow parallel edges
      inp->edges.insert(np->position);
    }
  }
  // Check if degree one
  if (np->edges.size() == 1) {
    insert_degone(np.get());
  }
}

template <class Symbol, class SymbolTraits>
void mp_context<Symbol,SymbolTraits>::decode_degree_one() {
  std::size_t ripple_inserted = 0;
  // Process each output node in the degone list
  for (node *np = degone_first; np != nullptr; np = np->next) {
    node *inp = inputs[*(np->edges.begin())].get();
    if (symbol_traits::is_empty(inp->symbol)) { // Not already decoded
      symbol_traits::swap(inp->symbol, np->symbol);
      ++decoded_count_;
      insert_ripple(inp);
      ++ripple_inserted;
    }
    // Remove the edge
    np->edges.clear();
    inp->edges.erase(np->position); // Ok with parallel edges too
  }
  // All this outputs have now degree 0. Clear the list
  degone_first = nullptr;
  degone_last = nullptr;
  _ripple_size.add_sample(ripple_inserted);
}

template <class Symbol, class SymbolTraits>
void mp_context<Symbol,SymbolTraits>::process_ripple() {
  // Iterate over the ripple
  for (node *inp = ripple_first; inp != nullptr; inp = inp->next) {
    // Follow each edge
    for (auto i = inp->edges.cbegin(); i != inp->edges.cend(); ++i) {
      node *outp = outputs[*i].get();
      // Update the output symbol
      symbol_traits::inplace_xor(outp->symbol, inp->symbol);
      // Remove the edge (in <- out)
      auto e = outp->edges.find(inp->position);
      outp->edges.erase(e); // Delete only one edge
      // Update degree one list
      if (outp->edges.size() == 1) {
	insert_degone(outp);
      }
      else if (outp->edges.size() == 0) {
	remove_degone(outp);
      }
    }
    // Remove all edges (in -> out)
    inp->edges.clear();
  }
  // Clear the ripple
  ripple_first = nullptr;
}

template <class Symbol, class SymbolTraits>
void mp_context<Symbol,SymbolTraits>::run() {
  using namespace std::chrono;

  if (has_decoded()) {
    last_run_time = duration<double>::zero();
    return;
  }

  auto t = high_resolution_clock::now();

  for (;;) {
    decode_degree_one();
    if (has_decoded() || ripple_first == nullptr) {
      ripple_first = nullptr; // Guarantee empty ripple at return
      break;
    }
    process_ripple();
  }

  last_run_time = high_resolution_clock::now() - t;
}

template <class Symbol, class SymbolTraits>
std::size_t mp_context<Symbol,SymbolTraits>::input_size() const {
  return inputs.size();
}

template <class Symbol, class SymbolTraits>
std::size_t mp_context<Symbol,SymbolTraits>::output_size() const {
  return outputs.size();
}

template <class Symbol, class SymbolTraits>
std::size_t mp_context<Symbol,SymbolTraits>::decoded_count() const {
  return decoded_count_;
}

template <class Symbol, class SymbolTraits>
bool mp_context<Symbol,SymbolTraits>::has_decoded() const {
  return decoded_count_ == inputs.size();
}

template <class Symbol, class SymbolTraits>
typename mp_context<Symbol,SymbolTraits>::inputs_iterator
mp_context<Symbol,SymbolTraits>::input_symbols_begin() const {
  return inputs_iterator(inputs.cbegin(), node2sym());
}

template <class Symbol, class SymbolTraits>
typename mp_context<Symbol,SymbolTraits>::inputs_iterator
mp_context<Symbol,SymbolTraits>::input_symbols_end() const {
  return inputs_iterator(inputs.cend(), node2sym());
}

template <class Symbol, class SymbolTraits>
typename mp_context<Symbol,SymbolTraits>::decoded_iterator
mp_context<Symbol,SymbolTraits>::decoded_symbols_begin() const {
  return decoded_iterator(input_symbols_begin(),
			  input_symbols_end());
}

template <class Symbol, class SymbolTraits>
typename mp_context<Symbol,SymbolTraits>::decoded_iterator
mp_context<Symbol,SymbolTraits>::decoded_symbols_end() const {
  return decoded_iterator(input_symbols_end(),
			  input_symbols_end());
}

template <class Symbol, class SymbolTraits>
void mp_context<Symbol,SymbolTraits>::insert_degone(node *np) {
  // Insert at the end
  if (degone_first != nullptr) {
    np->prev = degone_last;
    np->next = nullptr;
    degone_last = np;
    np->prev->next = np;
  }
  else { // Empty list
    degone_first = np;
    degone_last = np;
    np->next = nullptr;
    np->prev = nullptr;
  }
}

template <class Symbol, class SymbolTraits>
void mp_context<Symbol,SymbolTraits>::remove_degone(node *np) {
  // Remove from middle
  if (degone_first != np && degone_last != np) {
    np->next->prev = np->prev;
    np->prev->next = np->next;
  }
  // Remove from head
  else if (degone_first == np && degone_last != np) {
    np->next->prev = nullptr;
    degone_first = np->next;
  }
  // Remove from tail
  else if (degone_first != np && degone_last == np) {
    np->prev->next = nullptr;
    degone_last = np->prev;
  }
  // Is the only element
  else {
    degone_first = nullptr;
    degone_last = nullptr;
  }
}

template <class Symbol, class SymbolTraits>
void mp_context<Symbol,SymbolTraits>::insert_ripple(node *np) {
  np->next = ripple_first;
  ripple_first = np;
}

template <class Symbol, class SymbolTraits>
void mp_context<Symbol,SymbolTraits>::reset() {
  ripple_first = nullptr;
  degone_first = nullptr;
  degone_last = nullptr;
  decoded_count_ = 0;
  last_run_time = std::chrono::duration<double>::zero();
  for (std::unique_ptr<node> &n : inputs) {
    n->symbol = symbol_traits::create_empty();
    n->edges.clear();
  }
  outputs.clear();
  _ripple_size.reset();
}

template <class Symbol, class SymbolTraits>
double mp_context<Symbol,SymbolTraits>::run_duration() const {
  return last_run_time.count();
}

template<typename Symbol, typename SymbolTraits>
double mp_context<Symbol,SymbolTraits>::average_ripple_size() const {
  return _ripple_size.value();
}

}}

#endif
