#include "rng.hpp"

#include <iostream>

using namespace std;

int main(int, char**) {
  fountain<> f(10);
  for (int i = 0; i < 100; i++) {
    fountain<>::selection_type ps = f.next_packet_selection();
    for (auto j = ps.begin(); j != ps.end(); j++) {
      cout << *j << ' ';
    }
    cout << endl;
  }
}
