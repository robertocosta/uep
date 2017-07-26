#ifndef UEP_MESSAGE_PASSING
#define UEP_MESSAGE_PASSING

#include <functional>
#include <list>
#include <set>
#include <utility>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/iterator/transform_iterator.hpp>

namespace uep {

/** Class used to execute the message-passing algorithm.
 * The generic symbol type T must be DefaultConstructible,
 *  CopyAssignable, Swappable and the expression (a ^= b) must be
 *  valid when a,b are objects of type T, either given as output
 *  symbols or produced by successive invocations of "^=".
 */
template <class T>
class message_passing_context {
  struct index_comp;
  struct index_map;
  struct v2pair_conv;
  struct v2sym_conv;
  struct vertex_prop;
public:
  typedef T symbol_type;
  typedef std::pair<std::size_t, const T&> decoded_value_type;
private:
  typedef boost::adjacency_list<boost::listS,
				boost::vecS,
				boost::undirectedS,
				vertex_prop> graph_t;
  typedef typename graph_t::vertex_descriptor vdesc;
  typedef std::list<vdesc> deglist_t;
  typedef std::set<vdesc, index_comp> decoded_t;
  typedef std::vector<vdesc> ripple_t;
public:
  typedef boost::transform_iterator<v2pair_conv,
				    typename decoded_t::iterator
				    > decoded_iterator;
  typedef boost::transform_iterator<v2sym_conv,
				    typename decoded_t::iterator
				    > decoded_symbols_iterator;

  message_passing_context(const message_passing_context&) = default;
  message_passing_context(message_passing_context&&) = default;
  message_passing_context &operator=(const message_passing_context&) = default;
  message_passing_context &operator=(message_passing_context&&) = default;

  message_passing_context() : K(0), decoded(index_comp(&the_graph)) {}
  /** Construct a context with in_size default-constructed input symbols. */
  explicit message_passing_context(std::size_t in_size);
  /** Construct a context with in_size input_symbols and the given
   *  output symbols.
   */
  template <class SymIter>
  explicit message_passing_context(std::size_t in_size,
				   SymIter first_out, SymIter last_out);
  /** Construct a context with in_size default-constructed input
   *  symbols, output symbols copied from [first_out,last_out) and
   *  edges copied from [first_edge,last_edge).
   *  The edges must be pairs (i,j) of std::size_t, where i is an
   *  input symbol number in [0,K), j is an output symbol number in
   *  [0,N) and N = last_out - first_out.
   */
  template <class OutSymIter, class EdgeIter>
  explicit message_passing_context(std::size_t in_size,
				   OutSymIter first_out,
				   OutSymIter last_out,
				   EdgeIter first_edge,
				   EdgeIter last_edge);

  /** Run the message-passing algorithm with the current context.
   *  After the input symbols are fully decoded, this method does
   *  nothing.  If the input symbols are partially decoded, they are
   *  still available via decoded_begin(), decoded_end() and
   *  successive invocations of run() try to decode more symbols.
   */
  void run();

  // Implement if incremental construction is needed
  // void add_output_symbol(const T &sym);
  // void add_input_symbol();

  /** Add an edge that links input symbol i to output symbol j.
   *  Both symbols must already exist.
   */
  void add_edge(std::size_t i, std::size_t j) {
    const index_map m(&K);
    add_edge_vdesc(boost::vertex(i, the_graph),
		   boost::vertex(m(j), the_graph));
  }

  /** Return the number of input symbols. */
  std::size_t input_size() const { return K; }
  /** Return the number of output symbols. */
  std::size_t output_size() const { return boost::num_vertices(the_graph) - K; }
  /** Return the number of input symbols that have been correctly
   *  decoded up to the last call to run().
   */
  std::size_t decoded_count() const { return decoded.size(); }
  /** Return true when all the input symbols have been decoded. */
  bool has_decoded() const { return decoded_count() == input_size(); }

  /** Return a constant iterator to the start of the decoded (index,
   *  symbol) pairs.
   */
  decoded_iterator decoded_begin() const {
    const v2pair_conv c(&the_graph);
    return decoded_iterator(decoded.cbegin(), c);
  }

  /** Return a constant iterator to the end of the decoded (index,
   *  symbol) pairs.
   */
  decoded_iterator decoded_end() const {
    const v2pair_conv c(&the_graph);
    return decoded_iterator(decoded.cend(), c);
  }

  /** Return a constant iterator to the start of the decoded symbols. */
  decoded_symbols_iterator decoded_symbols_begin() const {
    const v2sym_conv c(&the_graph);
    return decoded_symbols_iterator(decoded.cbegin(), c);
  }

  /** Return a constant iterator to the end of the decoded symbols. */
  decoded_symbols_iterator decoded_symbols_end() const {
    const v2sym_conv c(&the_graph);
    return decoded_symbols_iterator(decoded.cend(), c);
  }

  /** Reset the message_passing_context to the initial state. */
  void clear() {
    K = 0;
    the_graph.clear();
    deglist.clear();
    decoded.clear();
    ripple.clear();
  }

private:
  std::size_t K;
  graph_t the_graph;
  deglist_t deglist;
  decoded_t decoded;
  ripple_t ripple;

  void add_edge_vdesc(vdesc u, vdesc v);
  void decode_degree_one();
  void process_ripple();
  void add_to_deglist(vdesc u);
  void remove_from_deglist(vdesc u);
};

/** Class to hold the vertex properties. I.e. the vertex index, the
 *  associated symbol and an iterator to the corresponding vertex
 *  descriptor in the degree-one list.
 */
template <class T>
struct message_passing_context<T>::vertex_prop {
  T symbol;
  std::size_t index;
  //If std::list this is invalidated only on deletion
  typename deglist_t::iterator deglist_iter;

  vertex_prop() = default;
  explicit vertex_prop(std::size_t i) :
    index(i) {}
  explicit vertex_prop(std::size_t i, const T &s) :
    symbol(s), index(i) {}
};

/** Converter from vertex descriptors to (index, symbol) pairs. */
template <class T>
struct message_passing_context<T>::v2pair_conv {
  const graph_t *the_graph;

  v2pair_conv() : the_graph(nullptr) {}
  explicit v2pair_conv(const graph_t *g) : the_graph(g) {}

  decoded_value_type operator()(vdesc v) const {
    return decoded_value_type((*the_graph)[v].index,
			      (*the_graph)[v].symbol);
  }
};

/** Converter from vertex descriptors to symbols. */
template <class T>
struct message_passing_context<T>::v2sym_conv {
  const graph_t *the_graph;

  v2sym_conv() : the_graph(nullptr) {}
  explicit v2sym_conv(const graph_t *g) : the_graph(g) {}

  const T &operator()(vdesc v) const {
    return (*the_graph)[v].symbol;
  }
};

/** Comparator for vertex descriptors. Applies < on the vertices' indices. */
template <class T>
struct message_passing_context<T>::index_comp {
  const graph_t *the_graph;

  index_comp() : the_graph(nullptr) {}
  explicit index_comp(const graph_t *g) : the_graph(g) {}

  bool operator()(vdesc lhs, vdesc rhs) const {
    std::size_t lhs_i = (*the_graph)[lhs].index;
    std::size_t rhs_i = (*the_graph)[rhs].index;
    return lhs_i < rhs_i;
  }
};

/** Mapper from (input_index, output_index) pairs to a pair of indices
 *  as stored by the graph class.
 */
template <class T>
struct message_passing_context<T>::index_map {
  const std::size_t *in_size;

  index_map() : in_size(nullptr) {}
  explicit index_map(const std::size_t *s) : in_size(s) {}

  std::pair<std::size_t,std::size_t>
  operator()(std::pair<std::size_t,std::size_t> e) const {
    e.second += *in_size;
    return e;
  }

  std::size_t operator()(std::size_t out_index) const {
    return out_index + *in_size;
  }
};

//		message_passing_context<T> definitions

template <class T>
message_passing_context<T>::message_passing_context(std::size_t in_size) :
  message_passing_context() {
  K = in_size;
  for (std::size_t i = 0; i < K; ++i) {
    boost::add_vertex(vertex_prop(i), the_graph);
  }
}

template <class T>
template <class SymIter>
message_passing_context<T>::message_passing_context(std::size_t in_size,
						    SymIter first_out,
						    SymIter last_out) :
  message_passing_context(in_size) {
  std::size_t i = boost::num_vertices(the_graph);
  while (first_out != last_out) {
    boost::add_vertex(vertex_prop(i++, *first_out++), the_graph);
  }
}

template <class T>
template <class OutSymIter, class EdgeIter>
message_passing_context<T>::message_passing_context(std::size_t in_size,
						    OutSymIter first_out,
						    OutSymIter last_out,
						    EdgeIter first_edge,
						    EdgeIter last_edge) :
  message_passing_context() {
  using namespace std;
  using namespace std::placeholders;
  using namespace boost;

  K = in_size;

  // Init the graph with the edges (also transform (i,j) -> (i, j+K))
  const index_map imap(&K);
  typedef transform_iterator<index_map, EdgeIter> tx_edge_iter;
  tx_edge_iter first_e_tx(first_edge, imap), last_e_tx(last_edge, imap);
  size_t out_size = last_out - first_out;
  the_graph = graph_t(first_e_tx, last_e_tx, K + out_size);

  // Setup the indices, copy the symbols
  auto vr = vertices(the_graph);
  size_t i = 0;
  while (i < K) {
    the_graph[*vr.first].index = i;
    ++i;
    ++vr.first;
  }
  while (vr.first != vr.second) {
    the_graph[*vr.first].symbol = *first_out;
    the_graph[*vr.first].index = i;
    // Setup the degree-one list
    if (out_degree(*vr.first, the_graph) == 1) {
      deglist.push_back(*vr.first);
      the_graph[*vr.first].deglist_iter = --deglist.end();
    }
    ++vr.first;
    ++first_out;
    ++i;
  }
}

template <class T>
void message_passing_context<T>::decode_degree_one() {
  using std::swap;
  using namespace boost;

  ripple.reserve(deglist.size());
  for (auto i = deglist.cbegin(); i != deglist.cend(); ++i) {
    auto adj_inp = *(adjacent_vertices(*i, the_graph).first);
    auto dec_res = decoded.insert(adj_inp);
    if (dec_res.second) {
      ripple.push_back(adj_inp);
      swap(the_graph[*i].symbol, the_graph[adj_inp].symbol);
    }
    remove_edge(*i, adj_inp, the_graph);
    the_graph[*i].deglist_iter = typename deglist_t::iterator();
  }
  deglist.clear();
}

template <class T>
void message_passing_context<T>::process_ripple() {
  using namespace std;
  using namespace boost;

  for (auto i = ripple.cbegin(); i != ripple.cend(); ++i) {
    const T &input_sym = the_graph[*i].symbol;
    auto adj_r = adjacent_vertices(*i, the_graph);
    vector<typename graph_t::edge_descriptor> edges_to_delete;
    edges_to_delete.reserve(out_degree(*i, the_graph));
    for (auto j = adj_r.first; j != adj_r.second; ++j) {
      the_graph[*j].symbol ^= input_sym;
      auto e = edge(*i, *j, the_graph);
      edges_to_delete.push_back(e.first);
      size_t deg_before = out_degree(*j, the_graph);
      if (deg_before == 2) {
	deglist.push_back(*j);
	the_graph[*j].deglist_iter = --deglist.end();
      }
      else if (deg_before == 1) {
	auto iter = the_graph[*j].deglist_iter;
	the_graph[*j].deglist_iter = typename deglist_t::iterator();
	deglist.erase(iter);
      }
    }
    for (auto j = edges_to_delete.begin(); j != edges_to_delete.end(); ++j) {
      remove_edge(*j, the_graph);
    }
  }
  ripple.clear();
}

template <class T>
void message_passing_context<T>::run() {
  if (has_decoded()) return;

  ripple.clear();
  for (;;) {
    decode_degree_one();
    if (ripple.empty() || has_decoded()) break;
    process_ripple();
  }
}

template <class T>
void message_passing_context<T>::add_edge_vdesc(vdesc u, vdesc v) {
  using namespace std;
  using namespace boost;

  size_t deg_before = out_degree(v, the_graph);
  // Check parallel edges??
  boost::add_edge(u, v, the_graph);
  if (deg_before == 0) add_to_deglist(v);
  else if (deg_before == 1) remove_from_deglist(v);
}

template <class T>
void message_passing_context<T>::add_to_deglist(vdesc u) {
  deglist.push_back(u);
  the_graph[u].deglist_iter = --deglist.end();
}

template <class T>
void message_passing_context<T>::remove_from_deglist(vdesc u) {
  auto iter = the_graph[u].deglist_iter;
  the_graph[u].deglist_iter = decltype(iter)();
  deglist.erase(iter);
}

namespace mp {

/** Allow the use of the shorter name `uep::mp::mp_context`. */
template <class Symbol>
using mp_context = typename uep::message_passing_context<Symbol>;

}}

#endif
