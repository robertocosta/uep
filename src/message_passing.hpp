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
  typedef std::function<bool(vdesc,vdesc)> index_comparator_t;
  typedef std::set<vdesc, index_comparator_t> decoded_t;
  typedef std::vector<vdesc> ripple_t;
  typedef std::function<const T&(vdesc)> vertex_converter_t;
  typedef std::function<decoded_value_type(vdesc)> pair_converter_t;
public:
  typedef boost::transform_iterator<pair_converter_t,
				    typename decoded_t::iterator
				    > decoded_iterator;
  typedef boost::transform_iterator<vertex_converter_t,
				    typename decoded_t::iterator
				    > decoded_symbols_iterator;

  message_passing_context(const message_passing_context&) = default;
  message_passing_context(message_passing_context&&) = default;
  message_passing_context &operator=(const message_passing_context&) = default;
  message_passing_context &operator=(message_passing_context&&) = default;

  message_passing_context() : K(0), decoded(make_index_less()),
			      vertex_conv(make_vertex_conv()),
			      pair_conv(make_pair_conv()) {
  }

  /** Construct a context with in_size default-constructed input symbols. */
  message_passing_context(std::size_t in_size) : message_passing_context() {
    K = in_size;
    the_graph = graph_t(K);
  }

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
  // void add_edge(std::size_t i, std::size_t j);

  std::size_t input_size() const { return K; }
  std::size_t output_size() const { return boost::num_vertices(the_graph) - K; }
  std::size_t decoded_count() const { return decoded.size(); }
  bool has_decoded() const { return decoded_count() == input_size(); }

  decoded_iterator decoded_begin() const {
    return decoded_iterator(decoded.cbegin(), pair_conv);
  }

  decoded_iterator decoded_end() const {
    return decoded_iterator(decoded.cend(), pair_conv);
  }

  decoded_symbols_iterator decoded_symbols_begin() const {
    return decoded_symbols_iterator(decoded.cbegin(), vertex_conv);
  }

  decoded_symbols_iterator decoded_symbols_end() const {
    return decoded_symbols_iterator(decoded.cend(), vertex_conv);
  }

  void clear() {
    K = 0;
    the_graph.clear();
    deglist.clear();
    decoded.clear();
  }

private:
  std::size_t K;
  graph_t the_graph;
  deglist_t deglist;
  decoded_t decoded;
  ripple_t ripple;
  vertex_converter_t vertex_conv;
  pair_converter_t pair_conv;


  // typename graph_t::vertex_descriptor input_vertex(std::size_t pos) const {
  //   return boost::vertex(pos, the_graph);
  // }

  // typename graph_t::vertex_descriptor output_vertex(std::size_t pos) const {
  //   return boost::vertex(K + pos, the_graph);
  // }

  void decode_degree_one();
  void process_ripple();

  std::function<bool(typename graph_t::vertex_descriptor,
		     typename graph_t::vertex_descriptor)>
  make_index_less() const {
    return
      [this](typename graph_t::vertex_descriptor lhs,
	     typename graph_t::vertex_descriptor rhs) -> bool {
      std::size_t lhs_i = the_graph[lhs].index;
      std::size_t rhs_i = the_graph[rhs].index;
      return lhs_i < rhs_i;
    };
  }

  pair_converter_t make_pair_conv() const {
    return
      [this](typename graph_t::vertex_descriptor v) -> decoded_value_type {
      return decoded_value_type(the_graph[v].index, the_graph[v].symbol);
    };
  }

  vertex_converter_t make_vertex_conv() const {
    return
      [this](typename graph_t::vertex_descriptor v) -> const T& {
      return the_graph[v].symbol;
    };
  }
};

template <class T>
struct message_passing_context<T>::vertex_prop {
  T symbol;
  std::size_t index;
  //If std::list this is invalidated only on deletion
  typename deglist_t::iterator deglist_iter;
};

// Template definitions

template <class T>
template <class OutSymIter, class EdgeIter>
message_passing_context<T>::message_passing_context(std::size_t in_size,
						    OutSymIter first_out,
						    OutSymIter last_out,
						    EdgeIter first_edge,
						    EdgeIter last_edge) :
  message_passing_context(in_size) {
  using namespace std;
  using namespace std::placeholders;
  using namespace boost;

  // Init the graph with the edges (also transform (i,j) -> (i, j+K))
  auto tx_e = [in_size](pair<size_t,size_t> e) -> pair<size_t,size_t> {
    e.second += in_size;
    return e;
  };
  typedef transform_iterator<decltype(tx_e), EdgeIter> tx_edge_iter;
  tx_edge_iter first_e_tx(first_edge, tx_e), last_e_tx(last_edge, tx_e);
  size_t out_size = last_out - first_out;
  the_graph = graph_t(first_e_tx, last_e_tx, K + out_size);

  // Copy the output symbols into the graph
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

}

#endif
