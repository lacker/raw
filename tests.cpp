#include <iostream>

#include "raw.h"

using namespace std;

// Just runs some tests
int main(int argc, char* argv[]) {
  if (argc != 2) {
    cerr << "usage: tests <file.raw>\n";
    exit(1);
  }
  string filename(argv[1]);
  cout << "hello raw testing world " << filename << endl;
}
