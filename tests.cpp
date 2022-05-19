#include <fcntl.h>
#include <iostream>
#include <vector>

#include "raw.h"

using namespace std;

// Just runs some tests
int main(int argc, char* argv[]) {
  if (argc != 2) {
    cerr << "usage: tests <file.raw>\n";
    exit(1);
  }
  string filename(argv[1]);
  cout << "running tests on " << filename << endl;

  int num_blocks = 0;
  raw::Reader reader(filename);
  raw::Header header;
  while (reader.readHeader(&header)) {
    // Read only the odd blocks
    if (num_blocks % 2 == 1) {
      std::vector<char> data(header.blocsize);
      reader.readData(data.data());
    }

    ++num_blocks;
    if (num_blocks % 10 == 0) {
      cout << "processed " << num_blocks << " blocks\n";
    }
  }
  cout << "done. processed " << num_blocks << " blocks total\n";
  
  cout << "OK" << endl;
}
