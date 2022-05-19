#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "header.h"
#include "util.h"

namespace raw {

  class Reader {

  private:
    int fdin;
    
  public:

    std::string filename;
    
    Reader(const std::string& filename) : filename(filename) {
      fdin = open(filename.c_str(), O_RDONLY);
      posix_fadvise(fdin, 0, 0, POSIX_FADV_SEQUENTIAL);
    }

    ~Reader() {
      close(fdin);
    }

    // Reads a header and the block.
    // Returns false if we're at the end of the file.
    // Exits if we run into any one of a number of errors.
    bool process() {
      raw::Header header;
      auto pos = rawspec_raw_read_header(fdin, &header);
      if (pos <= 0) {
	if (pos == -1) {
	  std::cerr << "error getting obs params from " << filename << "\n";
	} else {
	  // We're at the end of the file.
	  return false;
	}
	exit(1);
      }

      // Verify that obsnchan is divisible by nants
      if (header.obsnchan % header.nants != 0) {
	std::cerr << "bad obsnchan/nants: " << header.obsnchan << " % " << header.nants << " != 0\n";
	exit(1);
      }

      if (header.nbits != 8) {
	std::cerr << "the raw library can currently only handle nbits = 8\n";
	exit(1);
      }
  
      // Validate block dimensions.
      // The 2 is because we store both real and complex values.
      int bits_per_timestep = 2 * header.npol * header.obsnchan * header.nbits;
      int bytes_per_timestep = bits_per_timestep / 8;
      if (header.blocsize % bytes_per_timestep != 0) {
	std::cerr << "invalid block dimensions: blocsize " << header.blocsize << " is not divisible by " << bytes_per_timestep << std::endl;
	exit(1);
      }
      int num_timesteps = header.blocsize / bytes_per_timestep;

      std::vector<char> data(header.blocsize);
      auto bytes_read = read_fully(fdin, data.data(), data.size());
      if (bytes_read < 0) {
	std::cerr << "error while reading file\n";
	exit(1);
      }
      if (bytes_read < data.size()) {
	std::cerr << "incomplete block at end of file\n";
	exit(1);
      }

      // The read was successful
      return true;
    }
  };


}
