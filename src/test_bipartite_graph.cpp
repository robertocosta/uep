#include "bipartite_graph.hpp"

#include <iostream>
#include <iterator>
#include <string>

using namespace std;

int main(int, char**) {
  vector<int> v {1,2,3,4,5};
  auto i = v.begin();
  auto end = v.end();
  bidi_iter_wrapper<vector<int>::iterator> iw(i);
  bidi_iter_wrapper<vector<int>::iterator> iw_end(end);
  for (;iw != iw_end; ) {
    cout << *iw++ << endl;
  }

  reverse_iterator<bidi_iter_wrapper<vector<int>::iterator> > rbegin(bidi_iter_wrapper<vector<int>::iterator>(v.end()));
  reverse_iterator<bidi_iter_wrapper<vector<int>::iterator> > rend(bidi_iter_wrapper<vector<int>::iterator>(v.begin()));

  // reverse_iterator<vector<int>::iterator> rbegin(v.end());
  // reverse_iterator<vector<int>::iterator> rend(v.begin());
  
  for(;rbegin != rend;) {
     cout << *rbegin++ << endl;
  }

  symmetric_multimap<double,double> mm;

  mm.insert(1, 1.0);
  mm.insert(4, 2.5);
  mm.insert(9, 2.5);
  auto ins_ret2 = mm.insert(1, 1.5);
  auto ins_ret = mm.insert(2, 2.5);

  int count = mm.erase_key(1);
  mm.erase(ins_ret.second);

  int count2 = mm.erase_value(2.5);

  bipartite_graph<string> bg;
  bg.resize_output(4);
  bg.output_at(0) = "a";
  bg.output_at(1) = "b";
  bg.output_at(3) = "c";
  bg.resize_input(5);
  bg.input_at(0) = "d";
  bg.input_at(2) = "e";
  bg.input_at(4) = "f";
    
  bg.add_link(0, 1);
  bg.add_link(0, 2);
  bg.add_link(2, 0);
  bg.add_link(2, 3);
  bg.add_link(4, 1);
  bg.add_link(4, 3);
  bool do_insert = bg.add_link(4,3);

  bg.remove_link(2, 3);
  bg.remove_link(0,0);
  
  return 0;
}
