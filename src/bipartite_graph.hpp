#ifndef BIPARTITE_GRAPH_HPP
#define BIPARTITE_GRAPH_HPP

#include <algorithm>
#include <cstddef>
#include <map>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

template<class T>
class bipartite_graph {
public:
  typedef typename std::vector<T>::size_type size_type;
  typedef typename std::set<size_type>::const_iterator const_output_degree_one_iterator;
  typedef typename std::set<size_type>::const_iterator const_index_iterator;
  typedef typename std::vector<T>::const_iterator const_iterator;
  typedef typename std::vector<T>::const_iterator const_output_iterator;
  typedef typename std::vector<T>::const_iterator const_input_iterator;
  
  void resize_input(size_type size) {
    input.resize(size);
    input_to_output.resize(size);
  }

  void resize_output(size_type size) {
    output.resize(size);
    output_to_input.resize(size);
  }
  
  T &input_at(size_type i) {
    return input.at(i);
  }

  T &output_at(size_type i) {
    return output.at(i);
  }

  bool add_link(size_type in, size_type out) {
    std::set<size_type> &in_to_out = input_to_output.at(in);
    std::set<size_type> &out_to_in = output_to_input.at(out);
    auto ins_ret = in_to_out.insert(out);
    bool inserted = ins_ret.second;
    if (inserted) {
      size_type prev_size = out_to_in.size();
      out_to_in.insert(in);
      if (prev_size == 1) {
	output_degree_one.erase(out);
      }
      else if (prev_size == 0) {
	output_degree_one.insert(out);
      }
    }
    return inserted;
  }

  template<class IterType>
  void add_links_to_output(size_type out, IterType begin, IterType end) {
    for (; begin != end; ++begin) {
      size_type in = *begin;
      add_link(in, out);
    }
  }

  void remove_link(size_type in, size_type out) {
    std::set<size_type> &in_to_out = input_to_output.at(in);
    std::set<size_type> &out_to_in = output_to_input.at(out);
    size_type prev_size = out_to_in.size();
    size_type deleted = out_to_in.erase(in);
    in_to_out.erase(out);
    if (deleted == 1) {
      if (prev_size == 1) {
	output_degree_one.erase(out);
      }
      else if (prev_size == 2) {
	output_degree_one.insert(out);
      }
    }
  }

  size_type find_input(size_type out) {
    const std::set<size_type> &out_to_in = output_to_input.at(out);
    if (out_to_in.empty()) throw std::out_of_range("Not connected");
    return *(out_to_in.begin());
  }

  size_type find_output(size_type in) {
    const std::set<size_type> &in_to_out = input_to_output.at(in);
    if (in_to_out.empty()) throw std::out_of_range("Not connected");
    return *(in_to_out.begin());
  }

  std::pair<const_index_iterator, const_index_iterator>
  find_all_outputs(size_type in) {
    const std::set<size_type> &in_to_out = input_to_output.at(in);
    return std::pair<const_index_iterator, const_index_iterator>(in_to_out.cbegin(), in_to_out.cend());						 
  }

  std::pair<const_index_iterator, const_index_iterator>
  find_all_inputs(size_type out) {
    const std::set<size_type> &out_to_in = output_to_input.at(out);
    return std::pair<const_index_iterator, const_index_iterator>(out_to_in.cbegin(), out_to_in.cend());						 
  }
  
  void clear() {
    output_to_input.clear();
    input_to_output.clear();
    output.clear();
    input.clear();
    output_degree_one.clear();
  }

  size_type output_size() const {
    return output.size();
  }
  
  size_type input_size() const {
    return input.size();
  }

  const_output_degree_one_iterator output_degree_one_begin() {
    return output_degree_one.cbegin();
  }

  const_output_degree_one_iterator output_degree_one_end() {
    return output_degree_one.cend();
  }

  const_iterator output_begin() {
    return output.cbegin();
  }

  const_iterator output_end() {
    return output.cend();
  }

  const_iterator input_begin() {
    return input.cbegin();
  }
  
  const_iterator input_end() {
    return input.cend();
  }
    
private:
  std::vector<T> output;
  std::vector<T> input;
  std::vector<std::set<size_type> > output_to_input;
  std::vector<std::set<size_type> > input_to_output;
  std::set<size_type> output_degree_one;
};

#endif
