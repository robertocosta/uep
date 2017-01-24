#include "rng.hpp"

#include <iostream>

using namespace std;

void test() {
  fountain<soliton_distribution> f(10);
  for (int i = 0; i < 100; i++) {
    fountain<soliton_distribution>::row_type ps = f.next_row();
    for (auto j = ps.begin(); j != ps.end(); j++) {
      cout << *j << ' ';
    }
    cout << endl;
  }
}

int main(int, char**) {
  test();
  return 0;
}
