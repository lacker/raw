#include <fcntl.h>
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
  cout << "running tests on " << filename << endl;

  auto fdin = open(filename.c_str(), O_RDONLY);
  posix_fadvise(fdin, 0, 0, POSIX_FADV_SEQUENTIAL);

  cerr << "fdin: " << fdin << endl;

  raw::rawspec_raw_hdr_t raw_hdr;
  auto pos = rawspec_raw_read_header(fdin, &raw_hdr);
  if (pos <= 0) {
    if (pos == -1) {
      cerr << "error getting obs params from " << filename << "\n";
    } else {
      cerr << "no data found in " << filename << "\n";
    }
    exit(1);
  }

  // Verify that obsnchan is divisible by nants
  if (raw_hdr.obsnchan % raw_hdr.nants != 0) {
    cerr << "bad obsnchan/nants: " << raw_hdr.obsnchan << " % " << raw_hdr.nants << " != 0\n";
    exit(1);
  }

  // Validate block dimensions
  int divisor = 2 * raw_hdr.npol * raw_hdr.obsnchan * raw_hdr.nbits / 8;
  if (raw_hdr.blocsize % divisor != 0) {
    cerr << "invalid block dimensions: blocsize " << raw_hdr.blocsize << " is not divisible by " << divisor << endl;
    exit(1);
  }
  int num_time_samples_per_block = raw_hdr.blocsize / divisor;
  cout << "ntpb " << num_time_samples_per_block << endl;
  
  close(fdin);
  cout << "OK" << endl;
}
