#ifndef UEP_MP_MESSAGE_PASSING_HPP
#define UEP_MP_MESSAGE_PASSING_HPP

#include <functional>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include <boost/iterator/transform_iterator.hpp>
#include <boost/shared_container_iterator.hpp>
#include <boost/shared_ptr.hpp>

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
 *  and operator^= must be defined for this type.
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

  /** Deleted copy constructor. */
  mp_context(const mp_context &other) = delete;
  /** Auto-generated move constructor. */
  mp_context(mp_context &&other) = default;

  /** Deleted copy-assignment operator. */
  mp_context &operator=(const mp_context &other) = delete;
  /** Auto-generated move-assignment operator. */
  mp_context &operator=(mp_context &&other) = default;

  /** Auto-generated destructor. */
  ~mp_context() = default;

  /** Add an output symbol to the bipartite graph, connected with the
   *  input symbols given by the iterator pair.
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
  std::vector<std::unique_ptr<node>> inputs; /**< Pointers to the input nodes. */
  std::vector<std::unique_ptr<node>> outputs; /**< Pointers to the output nodes. */
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
  node *next; /**< Pointer to the next node in a singly linked
	       *   list. Can be used either by the ripple or the
	       *   degone lists.
	       */
  node *prev; /**< Pointer to the previous node in a linked list. Is
	       *   used by the degone list.
	       */
  std::set<node*> edges; /**< Pointers to the nodes connected to this
			  *   one.
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
mp_context<Symbol>::mp_context(std::size_t in_size) : ripple_first(nullptr),
						      degone_first(nullptr),
						      degone_last(nullptr),
						      decoded_count_(0) {
  inputs.resize(in_size);
  for (auto i = inputs.begin(); i != inputs.end(); ++i) {
    auto np = std::make_unique<node>();
    np->symbol = symbol_type();
    *i = std::move(np);
  }
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
  auto npu = std::make_unique<node>();
  npu->symbol = std::move(s);
  node *np = npu.get();
  outputs.push_back(std::move(npu));
  // Add the edges
  for (EdgeIter i = edges_begin; i != edges_end; ++i) {
    node *inp = inputs.at(*i).get();
    auto ins_ret = np->edges.insert(inp);
    if (!ins_ret.second)
      throw std::logic_error("Setting a parallel edge on the output");
    inp->edges.insert(np);
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
    node *inp = *(np->edges.begin());
    if (!(inp->symbol)) { // Not already decoded
      swap(inp->symbol, np->symbol);
      ++decoded_count_;
      insert_ripple(inp);
    }
    // Remove the edge
    np->edges.clear();
    inp->edges.erase(np);
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
      node *outp = *i;
      // Update the output symbol
      outp->symbol ^= inp->symbol;
      // Remove the edge (in <- out)
      outp->edges.erase(inp);
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
  if (has_decoded()) return;

  for (;;) {
    decode_degree_one();
    if (has_decoded() || ripple_first == nullptr) {
      ripple_first = nullptr;
      break;
    }
    process_ripple();
  }
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
}

}}

#endif
