#include <fcntl.h>
#include <iostream>
#include <vector>

#include "raw.h"

using namespace std;

// Reads `bytes_to_read` bytes from `fd` into the buffer pointed to by `buf`.
// Returns the total bytes read or -1 on error.  A non-negative return value
// will be less than `bytes_to_read` only of EOF is reached.
ssize_t read_fully(int fd, char* buf, size_t bytes_to_read)
{
  ssize_t bytes_read;
  ssize_t total_bytes_read = 0;

  while(bytes_to_read > 0) {
    bytes_read = read(fd, buf, bytes_to_read);
    if(bytes_read <= 0) {
      if(bytes_read == 0) {
	break;
      } else {
	return -1;
      }
    }
    buf += bytes_read;
    bytes_to_read -= bytes_read;
    total_bytes_read += bytes_read;
  }

  return total_bytes_read;
}


// Reads a header and the block.
// Returns false if we're at the end of the file.
// Exits if we run into any one of a number of errors.
bool process(const string& filename, int fdin) {
  raw::rawspec_raw_hdr_t raw_hdr;
  auto pos = rawspec_raw_read_header(fdin, &raw_hdr);
  if (pos <= 0) {
    if (pos == -1) {
      cerr << "error getting obs params from " << filename << "\n";
    } else {
      // We're at the end of the file.
      return false;
    }
    exit(1);
  }

  // Verify that obsnchan is divisible by nants
  if (raw_hdr.obsnchan % raw_hdr.nants != 0) {
    cerr << "bad obsnchan/nants: " << raw_hdr.obsnchan << " % " << raw_hdr.nants << " != 0\n";
    exit(1);
  }

  if (raw_hdr.nbits != 8) {
    cerr << "the raw library can currently only handle nbits = 8\n";
    exit(1);
  }
  
  // Validate block dimensions
  int bytes_per_time_sample = 2 * raw_hdr.npol * raw_hdr.obsnchan * raw_hdr.nbits / 8;
  if (raw_hdr.blocsize % bytes_per_time_sample != 0) {
    cerr << "invalid block dimensions: blocsize " << raw_hdr.blocsize << " is not divisible by " << bytes_per_time_sample << endl;
    exit(1);
  }
  int num_time_samples_per_block = raw_hdr.blocsize / bytes_per_time_sample;
  // cout << "ntpb " << num_time_samples_per_block << endl;

  /*
  cerr << "BLOCSIZE = " << raw_hdr.blocsize << endl;
  cerr << "OBSNCHAN = " << raw_hdr.obsnchan << endl;
  cerr << "NANTS    = " << raw_hdr.nants << endl;
  cerr << "NBITS    = " << raw_hdr.nbits << endl;
  cerr << "NPOL     = " << raw_hdr.npol << endl;
  cerr << "OBSFREQ  = " << raw_hdr.obsfreq << endl;
  cerr << "OBSBW    = " << raw_hdr.obsbw << endl;
  cerr << "TBIN     = " << raw_hdr.tbin << endl;
  */

  vector<char> data(raw_hdr.blocsize);
  auto bytes_read = read_fully(fdin, data.data(), data.size());
  if (bytes_read < 0) {
    cerr << "error while reading file\n";
    exit(1);
  }
  if (bytes_read < data.size()) {
    cerr << "incomplete block at end of file\n";
    exit(1);
  }

  // The read was successful
  return true;
}

// Just runs some tests
int main(int argc, char* argv[]) {
  if (argc != 2) {
    cerr << "usage: tests <file.raw>\n";
    exit(1);
  }
  string filename(argv[1]);
  cout << "running tests on " << filename << endl;

  int fdin = open(filename.c_str(), O_RDONLY);
  posix_fadvise(fdin, 0, 0, POSIX_FADV_SEQUENTIAL);

  cerr << "fdin: " << fdin << endl;

  int num_blocks = 0;
  while (process(filename, fdin)) {
    ++num_blocks;
    if (num_blocks % 10 == 0) {
      cout << "processed " << num_blocks << " blocks\n";
    }
  }
  cout << "done. processed " << num_blocks << " blocks total\n";
  
  close(fdin);
  cout << "OK" << endl;
}
