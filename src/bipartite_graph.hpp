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

template<class It>
class bidi_iter_wrapper {
public:
  explicit bidi_iter_wrapper(It i) : iter(i) {
  }

  bool operator==(bidi_iter_wrapper other) const {
    return iter == other.iter;
  }

  bool operator!=(bidi_iter_wrapper other) const {
    return iter != other.iter;
  }

  void swap(bidi_iter_wrapper &other) {
    std::swap(iter, other.iter);
  }

  typename It::reference operator*() const {
    return *iter;
  }

  bidi_iter_wrapper &operator++() {
    ++iter;
    return *this;
  }

  bidi_iter_wrapper operator++(int) {
    bidi_iter_wrapper i(*this);
    ++(*this);
    return i;
  }

  bidi_iter_wrapper &operator--() {
    --iter;
    return *this;
  }

  bidi_iter_wrapper operator--(int) {
    bidi_iter_wrapper i(*this);
    --(*this);
    return i;
  }

  It unwrap() {
    return iter;
  }

protected:
  It iter;
};

namespace std {
  template<class It>
  class iterator_traits<bidi_iter_wrapper<It> > {
  private:
    typedef iterator_traits<It> wrapped_traits;
  public:
    typedef typename wrapped_traits::value_type value_type;
    typedef typename wrapped_traits::difference_type difference_type;
    typedef typename wrapped_traits::reference reference;
    typedef typename wrapped_traits::pointer pointer;
    typedef typename wrapped_traits::iterator_category iterator_category;
  };
}

template<class K, class V,
	 class CompareK = std::less<K>,
	 class CompareV = std::less<V> >
class symmetric_multimap {
private:
  typedef std::multimap<K,V,CompareK> mm_fwd_t;
  typedef std::multimap<V,K,CompareV> mm_back_t;

public:
  struct kv_iterator : public bidi_iter_wrapper<typename mm_fwd_t::iterator> {
    kv_iterator(typename mm_fwd_t::iterator i) :
      bidi_iter_wrapper<typename mm_fwd_t::iterator>(i) {
    }
  };
  
  struct vk_iterator : public bidi_iter_wrapper<typename mm_back_t::iterator> {
    vk_iterator(typename mm_back_t::iterator i) :
      bidi_iter_wrapper<typename mm_back_t::iterator>(i) {
    }
  };
  
  typedef typename mm_fwd_t::size_type size_type;

  symmetric_multimap() {
    const bool same_size_t = std::is_same<typename mm_fwd_t::size_type,
					  typename mm_back_t::size_type>::value;
    static_assert(same_size_t, "fwd and back maps have different size_types");
  }

  kv_iterator kv_begin() {
    return fwd.begin();
  }

  kv_iterator kv_end() {
    return fwd.end();
  }

  vk_iterator vk_begin() {
    return back.begin();
  }

  vk_iterator vk_end() {
    return back.end();
  }

  bool empty() const {
    return fwd.empty();
  }

  size_type size() const {
    return fwd.size();
  }

  void clear() {
    fwd.clear();
    back.clear();
  }

  std::pair<kv_iterator,vk_iterator> insert(const K &key, const V &value) {
    kv_iterator i = fwd.insert(std::pair<K,V>(key, value));
    vk_iterator j = back.insert(std::pair<V,K>(value, key));
    return std::pair<kv_iterator,vk_iterator>(i,j);
  }

  std::pair<kv_iterator, vk_iterator> erase(kv_iterator i) {
    typename mm_fwd_t::iterator iu = i.unwrap();
    V value = iu->second;
    auto back_range = back.equal_range(value);
    auto j = std::find(back_range.first, back_range.second,
		       typename mm_back_t::value_type(value, iu->first));
    vk_iterator vk_j = back.erase(j);
    kv_iterator kv_i = fwd.erase(iu);
    return std::pair<kv_iterator, vk_iterator>(kv_i, vk_j);
  }

  std::pair<kv_iterator, vk_iterator> erase(vk_iterator i) {
    typename mm_back_t::iterator iu = i.unwrap();
    K key = iu->second;
    auto fwd_range = fwd.equal_range(key);
    auto j = std::find(fwd_range.first, fwd_range.second,
		       typename mm_fwd_t::value_type(key, iu->first));
    kv_iterator kv_j = fwd.erase(j);
    vk_iterator vk_i = back.erase(iu);
    return std::pair<kv_iterator, vk_iterator>(kv_j, vk_i);
  }

  size_type erase_key(const K &key) {
    auto item_range = fwd.equal_range(key);
    size_type count = 0;
    auto i = item_range.first;
    while (i != item_range.second) {
      auto reverse_item_range = back.equal_range(i->second);
      auto j = std::find(reverse_item_range.first, reverse_item_range.second,
			 typename mm_back_t::value_type(i->second, key));
      back.erase(j);
      i = fwd.erase(i);
      count++;
    }
    return count;
  }

  size_type erase_value(const V &value) {
    auto item_range = back.equal_range(value);
    size_type count = 0;
    auto i = item_range.first;
    while (i != item_range.second) {
      auto reverse_item_range = fwd.equal_range(i->second);
      auto j = std::find(reverse_item_range.first, reverse_item_range.second,
			 typename mm_fwd_t::value_type(i->second, value));
      fwd.erase(j);
      i = back.erase(i);
      count++;
    }
    return count;
  }

  kv_iterator find_key(const K &key) {
    return fwd.find(key);
  }

  vk_iterator find_value(const V &value) {
    return back.find(value);
  }

  std::pair<kv_iterator,kv_iterator> equal_key_range(const K &key) {
    
  }
  
private:
  mm_fwd_t fwd;
  mm_back_t back;
};

template<class T>
class bipartite_graph {
public:
  typedef typename std::vector<T>::size_type size_type;
  typedef typename std::set<size_type>::const_iterator const_output_degree_one_iterator;
  typedef typename std::set<size_type>::const_iterator const_index_iterator;
  typedef typename std::vector<T>::const_iterator const_iterator;
  typedef typename std::vector<T>::const_iterator const_output_iterator;
  typedef typename std::vector<T>::const_iterator const_input_iterator;
  
  // class iterator {
  // public:
  //   explicit iterator(const bipartite_graph &graph_,
  // 		      const std::vector<T> &vec_,
  // 		      size_type pos_) :
  //     graph(graph_),
  //     vec(vec_),
  //     pos(pos_) {
  //   }

  //   bool operator==(iterator other) const {
  //     if (&vec != &(other.vec)) return false;
  //     else return pos == other.pos;
  //   }

  //   bool operator!=(iterator other) const {
  //     return !(*this == other);
  //   }

  //   T &operator*() const {
  //     return vec[pos];
  //   }

  //   iterator &operator++() {
  //     ++pos;
  //     return *this;
  //   }

  //   iterator operator++(int) {
  //     iterator i(*this);
  //     ++(*this);
  //     return i;
  //   }
    
  // private:
  //   const bipartite_graph &graph;
  //   const std::vector<T> &vec;
  //   size_type pos;
  // };
  
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
  // std::set<index_type> output_present;
  // std::set<index_type> input_present;
  // size_type output_size_;
  // size_type input_size_;

  // bool is_output_iter(iterator i) {
  //   return &(i.graph) == this && &(i.vec) == &output;
  // }

  // bool is_input_iter(iterator i) {
  //   return &(i.graph) == this && &(i.vec) == &input;
  // }
};


#endif
