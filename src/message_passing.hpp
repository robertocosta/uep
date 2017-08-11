#ifndef UEP_MP_MESSAGE_PASSING_HPP
#define UEP_MP_MESSAGE_PASSING_HPP

#include <algorithm>
#include <chrono>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/iterator/transform_iterator.hpp>

#include "log.hpp"
#include "utils.hpp"

namespace uep {
namespace mp {

/** Class used to execute the message-passing algorithm.
 *
 *  The context controls a bipartite graph, which is initialized with
 *  a fixed number of empty input symbols and is incrementally built
 *  from each output symbol, together with its edges.
 *
 *  The algorithm can be run many times without losing the previously
 *  decoded packets.
 *
 *  The generic symbol type `Symbol` must be DefaultConstructible,
 *  MoveAssignable and Swappable. It must also be convertible to bool
 *  and operator^= must be defined for this type.  If the mp_context
 *  is to be copied then the symbols must also be CopyConstructible
 *  and CopyAssignable.
 */
template <class Symbol>
class mp_context {
private:
  /** Struct used to carry additional information with the symbols. */
  struct node;
  /** Converter from node pointers to a const reference to their symbol. */
  struct node2sym;
public:
  /** The type used to represent the symbols. */
  typedef Symbol symbol_type;
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
  log::default_logger basic_lg, perf_lg;

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

template <class Symbol>
struct mp_context<Symbol>::node {
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
  std::unordered_set<std::size_t> edges; /**< Indices of the nodes
					  *   connected to this.
					  */
};

template <class Symbol>
struct mp_context<Symbol>::node2sym {
  const symbol_type &operator()(const std::unique_ptr<node> &np) const {
    return np->symbol;
  }
};

//	       mp_context<Symbol> template definitions

template <class Symbol>
mp_context<Symbol>::mp_context(std::size_t in_size) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  ripple_first(nullptr),
  degone_first(nullptr),
  degone_last(nullptr),
  decoded_count_(0),
  last_run_time(std::chrono::duration<double>::zero()) {
  // Build in_size empty input nodes
  inputs.resize(in_size);
  std::size_t pos = 0;
  for (auto i = inputs.begin(); i != inputs.end(); ++i) {
    auto np = std::make_unique<node>();
    np->symbol = symbol_type();
    np->position = pos++;
    *i = std::move(np);
  }
}

template <class Symbol>
mp_context<Symbol>::mp_context(mp_context &&other) :
  // Moving the loggers does not work, copy them
  //basic_lg(std::move(other.basic_lg)),
  //perf_lg(std::move(other.perf_lg)),
  basic_lg(other.basic_lg),
  perf_lg(other.perf_lg),
  inputs(std::move(other.inputs)),
  outputs(std::move(other.outputs)),
  ripple_first(std::move(other.ripple_first)),
  degone_first(std::move(other.degone_first)),
  degone_last(std::move(other.degone_last)),
  decoded_count_(std::move(other.decoded_count_)),
  last_run_time(std::move(other.last_run_time)) {
}

template <class Symbol>
mp_context<Symbol>::mp_context(const mp_context &other) :
  basic_lg(other.basic_lg),
  perf_lg(other.perf_lg),
  inputs(other.inputs.size()), // copied after
  outputs(other.outputs.size()), // copied after
  ripple_first(nullptr), // Is always null outside of run()
  degone_first(nullptr), // The list must be rebuilt
  degone_last(nullptr), // The list must be rebuilt
  decoded_count_(other.decoded_count_),
  last_run_time(other.last_run_time) {
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

template <class Symbol>
mp_context<Symbol> &mp_context<Symbol>::operator=(mp_context &&other) {
  // Moving the loggers does not work, copy them
  //basic_lg = std::move(other.basic_lg);
  //perf_lg = std::move(other.perf_lg);
  basic_lg = other.basic_lg;
  perf_lg = other.perf_lg;
  inputs = std::move(other.inputs);
  outputs = std::move(other.outputs);
  ripple_first = std::move(other.ripple_first);
  degone_first = std::move(other.degone_first);
  degone_last = std::move(other.degone_last);
  decoded_count_ = std::move(other.decoded_count_);
  last_run_time = std::move(other.last_run_time);
  return *this;
}

template <class Symbol>
mp_context<Symbol> &mp_context<Symbol>::operator=(const mp_context &other) {
  basic_lg = other.basic_lg;
  perf_lg = other.perf_lg;
  inputs.resize(other.inputs.size()); // copied after
  outputs.resize(other.outputs.size()); // copied after
  ripple_first = nullptr; // Is always null outside of run()
  degone_first = nullptr; // The list must be rebuilt
  degone_last = nullptr; // The list must be rebuilt
  decoded_count_ = other.decoded_count_;
  last_run_time = other.last_run_time;
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

template <class Symbol>
template <class EdgeIter>
void mp_context<Symbol>::add_output(const symbol_type &s,
				    EdgeIter edges_begin, EdgeIter edges_end) {
  symbol_type s_copy(s);
  add_output(std::move(s_copy), edges_begin, edges_end);
}

template <class Symbol>
template <class EdgeIter>
void mp_context<Symbol>::add_output(symbol_type &&s,
				    EdgeIter edges_begin, EdgeIter edges_end) {
  // Move the new symbol in an output node
  auto nup = std::make_unique<node>();
  nup->symbol = std::move(s);
  nup->position = outputs.size();
  node *np = nup.get();
  outputs.push_back(std::move(nup));
  // Add the edges
  for (EdgeIter i = edges_begin; i != edges_end; ++i) {
    node *inp = inputs.at(*i).get();
     if (inp->symbol) {
       // input is already decoded, ignore edge
       np->symbol ^= inp->symbol;
     }
     else {
       auto ins_ret = np->edges.insert(*i);
       if (!ins_ret.second)
	 throw std::logic_error("Setting a parallel edge on the output");
       inp->edges.insert(np->position);
     }
  }
  // Check if degree one
  if (np->edges.size() == 1) {
    insert_degone(np);
  }
}

template <class Symbol>
void mp_context<Symbol>::decode_degree_one() {
  using std::swap;
  // Process each output node in the degone list
  for (node *np = degone_first; np != nullptr; np = np->next) {
    node *inp = inputs[*(np->edges.begin())].get();
    if (!(inp->symbol)) { // Not already decoded
      swap(inp->symbol, np->symbol);
      ++decoded_count_;
      insert_ripple(inp);
    }
    // Remove the edge
    np->edges.clear();
    inp->edges.erase(np->position);
  }
  // All this outputs have now degree 0
  degone_first = nullptr;
  degone_last = nullptr;
}

template <class Symbol>
void mp_context<Symbol>::process_ripple() {
  // Iterate over the ripple
  for (node *inp = ripple_first; inp != nullptr; inp = inp->next) {
    // Follow each edge
    for (auto i = inp->edges.cbegin(); i != inp->edges.cend(); ++i) {
      node *outp = outputs[*i].get();
      // Update the output symbol
      outp->symbol ^= inp->symbol;
      // Remove the edge (in <- out)
      outp->edges.erase(inp->position);
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

template <class Symbol>
void mp_context<Symbol>::run() {
  using namespace std::chrono;

  if (has_decoded()) {
    last_run_time = duration<double>::zero();
    return;
  }

  auto t = high_resolution_clock::now();

  for (;;) {
    decode_degree_one();
    if (has_decoded() || ripple_first == nullptr) {
      ripple_first = nullptr;
      break;
    }
    process_ripple();
  }

  last_run_time = high_resolution_clock::now() - t;

  BOOST_LOG(perf_lg) << "mp_context::run run_duration="
		     << last_run_time.count()
		     << " decoded_count="
		     << decoded_count_
		     << " output_size="
		     << outputs.size();
}

template <class Symbol>
std::size_t mp_context<Symbol>::input_size() const {
  return inputs.size();
}

template <class Symbol>
std::size_t mp_context<Symbol>::output_size() const {
  return outputs.size();
}

template <class Symbol>
std::size_t mp_context<Symbol>::decoded_count() const {
  return decoded_count_;
}

template <class Symbol>
bool mp_context<Symbol>::has_decoded() const {
  return decoded_count_ == inputs.size();
}

template <class Symbol>
typename mp_context<Symbol>::inputs_iterator
mp_context<Symbol>::input_symbols_begin() const {
  return inputs_iterator(inputs.cbegin(), node2sym());
}

template <class Symbol>
typename mp_context<Symbol>::inputs_iterator
mp_context<Symbol>::input_symbols_end() const {
  return inputs_iterator(inputs.cend(), node2sym());
}

template <class Symbol>
typename mp_context<Symbol>::decoded_iterator
mp_context<Symbol>::decoded_symbols_begin() const {
  return decoded_iterator(input_symbols_begin(),
			  input_symbols_end());
}

template <class Symbol>
typename mp_context<Symbol>::decoded_iterator
mp_context<Symbol>::decoded_symbols_end() const {
  return decoded_iterator(input_symbols_end(),
			  input_symbols_end());
}

template <class Symbol>
void mp_context<Symbol>::insert_degone(node *np) {
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

template <class Symbol>
void mp_context<Symbol>::remove_degone(node *np) {
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

template <class Symbol>
void mp_context<Symbol>::insert_ripple(node *np) {
  np->next = ripple_first;
  ripple_first = np;
}

template <class Symbol>
void mp_context<Symbol>::reset() {
  decoded_count_ = 0;
  degone_first = nullptr;
  degone_last = nullptr;
  ripple_first = nullptr;
  for (auto i = inputs.begin(); i != inputs.end(); ++i) {
    (*i)->symbol = symbol_type();
    (*i)->edges.clear();
  }
  outputs.clear();
  last_run_time = std::chrono::duration<double>::zero();
}

template <class Symbol>
double mp_context<Symbol>::run_duration() const {
  if (last_run_time == decltype(last_run_time)::zero())
    throw std::runtime_error("No run duration measured");
  return last_run_time.count();
}

}}

#endif
