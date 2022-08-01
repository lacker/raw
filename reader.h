#pragma once

#include <fcntl.h>
#include <future>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
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
    int headers_read = 0;

    // The number of bytes in the block that fdin is currently pointed at.
    // Set to 0 before we have read any blocks
    int current_block_size = 0;

    // The amount that fdin is offset from the start of our current block.
    // Set to 0 before we have read any blocks
    int current_block_offset = 0;

    // The pktidx from the current block.
    // Set to 0 before we have read any blocks
    int64_t pktidx = 0;

    // Once err is used, the reader is in "error state".
    ErrorMessage err = ErrorMessage();
    
  public:
    std::string filename;
    
    Reader(const std::string& filename) : filename(filename) {
      fdin = open(filename.c_str(), O_RDONLY | O_DIRECT);
      // posix_fadvise(fdin, 0, 0, POSIX_FADV_SEQUENTIAL);
    }

    Reader(const Reader&) = delete;
    Reader& operator=(Reader&) = delete;
    
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
    // If readHeader returns false, it can either be an error, or we reached the end of
    // the file.
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

        if (headers_read == 0) {
          err << "could not open " << filename;
        } else {
          err << "error reading block header #" << (headers_read + 1) << " from "
              << filename;
        }
	return false;
      }      

      // Verify that obsnchan is divisible by nants
      if (header->obsnchan % header->nants != 0) {
	err << "bad obsnchan/nants: " << header->obsnchan << " % " << header->nants
            << " != 0";
	return false;
      }
      header->num_channels = header->obsnchan / header->nants;

      if (header->nbits != 8) {
	err << "the raw library can currently only handle nbits = 8";
	return false;
      }

      // Validate block dimensions.
      // The 2 is because we store both real and complex values.
      int bits_per_timestep = 2 * header->npol * header->obsnchan * header->nbits;
      int bytes_per_timestep = bits_per_timestep / 8;
      if (header->blocsize % bytes_per_timestep != 0) {
	err << "invalid block dimensions: blocsize " << header->blocsize
            << " is not divisible by " << bytes_per_timestep;
	return false;
      }

      pktidx = header->pktidx;
      
      header->num_timesteps = header->blocsize / bytes_per_timestep;
      current_block_size = header->blocsize;
      current_block_offset = 0;
      ++headers_read;
      return true;
    }

    // Reads all data from the current block into the buffer, advancing fdin.
    // Returns whether the read was successful.
    bool readData(char* buffer) {
      if (current_block_offset != 0) {
	err << "cannot readData when data from this block has already been read";
	return false;
      }
      auto bytes_read = read_fully(fdin, buffer, current_block_size);
      if (bytes_read < 0) {
	err << "error while reading file";
	return false;
      }
      if (bytes_read < current_block_size) {
	err << "incomplete block at end of file";
	return false;
      }
      current_block_offset += current_block_size;
      return true;
    }

    // Reads a subset of the data in this block, defined by a frequency subband.
    // Returns whether the read succeeded.
    // This works regardless of where fdin is pointing and does not modify fdin.
    bool readBand(const Header& header, int band, int num_bands, char* buffer) const {
      assert(0 == header.num_channels % num_bands);
      int channels_per_band = header.num_channels / num_bands;
      assert(band < num_bands);

      // The slowest-moving index in the data is the antenna. After that is the frequency.
      // So, each antenna-band pair contains this much contiguous bytes:
      int band_bytes = channels_per_band * header.num_timesteps * header.npol * 2;

      // Each antenna has preband_bytes before the band we're interested in
      int preband_bytes = band * band_bytes;

      char* dest = buffer;
      
      for (int antenna = 0; antenna < header.nants; ++antenna) {
        if (!pread_fully(fdin, dest, band_bytes,
                         header.data_offset + preband_bytes +
                         antenna * num_bands * band_bytes)) {
          return false;
        }
        dest += band_bytes;
      }
      return true;
    }

    // Like readBand but puts the individual read successes into futures.
    void readBandAsync(const Header& header, int band, int num_bands,
                       char* buffer, std::vector<std::future<bool> >* futures) const {
      assert(0 == header.num_channels % num_bands);
      int channels_per_band = header.num_channels / num_bands;
      assert(band < num_bands);

      // The slowest-moving index in the data is the antenna. After that is the frequency.
      // So, each antenna-band pair contains this much contiguous bytes:
      int band_bytes = channels_per_band * header.num_timesteps * header.npol * 2;

      // Each antenna has preband_bytes before the band we're interested in
      int preband_bytes = band * band_bytes;

      char* dest = buffer;
      
      for (int antenna = 0; antenna < header.nants; ++antenna) {
        auto fut = std::async(pread_fully, fdin, dest, band_bytes,
                              header.data_offset + preband_bytes +
                              antenna * num_bands * band_bytes);
        futures->push_back(move(fut));
        dest += band_bytes;
      }
    }
    
  };
}
