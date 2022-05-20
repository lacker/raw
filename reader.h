#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "error_message.h"
#include "header.h"
#include "util.h"

namespace raw {

  class Reader {

  private:
    // The descriptor of the .raw file we're reading.
    int fdin;

    // How many headers have already been read from this file
    int headers_read;

    // The number of bytes in the block that fdin is currently pointed at.
    // Set to 0 before we have read any blocks
    int current_block_size;

    // The amount that fdin is offset from the start of our current block.
    // Set to 0 before we have read any blocks
    int current_block_offset;

    // The pktidx from the current block.
    // Set to 0 before we have read any blocks
    int64_t pktidx;

    // Once err is used, the reader is in "error state".
    ErrorMessage err;
    
  public:
    std::string filename;
    
    Reader(const std::string& filename)
      : headers_read(0), current_block_size(0), current_block_offset(0), pktidx(0), filename(filename) {
      fdin = open(filename.c_str(), O_RDONLY);
      posix_fadvise(fdin, 0, 0, POSIX_FADV_SEQUENTIAL);
    }

    ~Reader() {
      close(fdin);
    }

    // Whether we have run into an error
    bool error() {
      return err.used;
    }

    // The string for the error message
    std::string errorMessage() {
      return err;
    }
    
    // Reads the next header, advancing the internal file descriptor to the start of the
    // subsequent data block.
    // Returns whether the read was successful.
    // If readHeader returns false, it can either be an error, or we reached the end of the file.
    // Callers should check reader.error() to see if there was an error.
    bool readHeader(Header* header) {
      if (error()) {
	return false;
      }

      if (headers_read > 0) {
	// We may have to advance fdin to get to the next block.
	int advance = current_block_size - current_block_offset;
	if (advance != 0) {
	  lseek(fdin, advance, SEEK_CUR);
	}
      }
      
      auto pos = rawspec_raw_read_header(fdin, header);
      if (pos <= 0) {
	if (pos != -1) {
	  // We're at the end of the file.
	  return false;
	}

	err << "error getting obs params from " << filename;
	return false;
      }      

      // Verify that obsnchan is divisible by nants
      if (header->obsnchan % header->nants != 0) {
	std::cerr << "bad obsnchan/nants: " << header->obsnchan << " % " << header->nants << " != 0\n";
	exit(1);
      }
      header->num_channels = header->obsnchan / header->nants;

      if (header->nbits != 8) {
	std::cerr << "the raw library can currently only handle nbits = 8\n";
	exit(1);
      }

      // Validate block dimensions.
      // The 2 is because we store both real and complex values.
      int bits_per_timestep = 2 * header->npol * header->obsnchan * header->nbits;
      int bytes_per_timestep = bits_per_timestep / 8;
      if (header->blocsize % bytes_per_timestep != 0) {
	std::cerr << "invalid block dimensions: blocsize " << header->blocsize << " is not divisible by " << bytes_per_timestep << std::endl;
	exit(1);
      }

      if (headers_read > 0) {
	header->missing_blocks = pktidx - header->pktidx - 1;
      } else {
	header->missing_blocks = 0;
      }
      pktidx = header->pktidx;
      
      header->num_timesteps = header->blocsize / bytes_per_timestep;
      current_block_size = header->blocsize;
      current_block_offset = 0;
      ++headers_read;
      return true;
    }

    // Reads all data from the current block into the buffer, advancing fdin.
    void readData(char* buffer) {
      if (current_block_offset != 0) {
	std::cerr << "cannot readData when data from this block has already been read\n";
	exit(1);
      }
      auto bytes_read = read_fully(fdin, buffer, current_block_size);
      if (bytes_read < 0) {
	std::cerr << "error while reading file\n";
	exit(1);
      }
      if (bytes_read < current_block_size) {
	std::cerr << "incomplete block at end of file\n";
	exit(1);
      }
      current_block_offset += current_block_size;
    }    
  };
}
