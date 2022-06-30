#pragma once

#include <assert.h>

#include "hget.h"

static_assert(sizeof(int32_t) == sizeof(int), "require normal-sized int");
static_assert(sizeof(int64_t) == sizeof(long), "require normal-sized long");

namespace raw {
  const int MAX_RAW_HEADER_SIZE = 25600;

  /*
    The Header contains the information we get from processing one block of the .raw file.

   The .raw file format is similar to the FITS format:
     https://fits.gsfc.nasa.gov/fits_primer.html
  
   However, the header does not include the fields that are required
   in FITS, and it does not store the data blocks in any of the ways
   that are supported by the FITS format, so you generally cannot
   expect to use libraries designed to read FITS files on a .raw
   file. The headers are formatted the same way, though, minus the
   missing required fields, so we can use some code originally written
   to parse FITS headers.
   
   Most of the fields of the Header map directly to FITS headers. Some
   of them are auxiliary information generated during the parsing process.

   You will need the data from headers to handle the data in each
   block. The format of the block is a four-dimensional row-major array, indexed by:

   data[antenna][frequency][time][polarity]

   where the dimensions are in the fields:
     nants, num_channels, num_timesteps, npol

   Each entry is 2*nbits bits, and for now we only support nbits=8, so
   it's two bytes. The first byte is real and the second byte is complex.
  */
  class Header {
  public:
    // A buffer of raw data that we parse the header out of.
    // Aligned to a 512-byte boundary so that it can be used with files opened with O_DIRECT.    
    char buffer[MAX_RAW_HEADER_SIZE] __attribute__ ((aligned (512)));

    int directio;

    // The "BLOCSIZE" FITS header.
    // This is the size of the following data segment, in bytes, not including directio padding.
    size_t blocsize;

    // The "NPOL" FITS header, with the exception that any 4 is
    // treated like a 2, for reasons that are lost in time.
    // This is the number of polarities for the data.
    unsigned int npol;

    // The "OBSNCHAN" FITS header.
    // This is the number of frequency channels for the data times the
    // number of antennas. The name is because it used to only refer
    // to the number of frequency channels, but the semantics changed when we
    // started recording data for multiple channels.
    unsigned int obsnchan;

    // The "NBITS" FITS header.
    // This is the number of bits used to store each real or complex values, as a signed int.
    // We only support nbits = 8.
    unsigned int nbits;

    // The "PKTIDX" FITS header.
    // This is an index that counts up through the file, to let us detect missing blocks.
    long pktidx;

    // The "OBSFREQ" FITS header.
    // This is the center frequency of the entire range of frequencies
    // stored in the file, in MHz.
    double obsfreq;

    // The "OBSBW" FITS header.
    // This is the width of the range of frequencies, in MHz. Negative
    // indicates a reversed frequency axis.
    double obsbw;

    // The "TBIN" FITS header.
    // This is the time resolution for the data, in seconds. So, seconds per timestep.
    double tbin;

    // The right ascension of the telescope, in hours.
    // This is derived from the "RA_STR" FITS header which is HH:MM:SSS.ssss
    double ra;

    // The declination of the telescope, in degrees.
    // This is derived from the "DEC_STR" FITS header which is DD:MM:SSS.ssss
    double dec;

    // The start time in MJD format.
    // This is synthesized from the "STT_IMJD" and "STT_SMJD" FITS headers.
    // This is only accurate to the nearest second.
    // It also (probably) indicates the start time of the overall file, rather than
    // this particular block. So, maybe avoid using this.
    double mjd;

    // The "BEAM_ID" FITS header.
    // The beam id. -1 is unknown or a single beam receiver.
    int beam_id;

    // The "NANTS" FITS header.
    // This is the number of antennas in the data.
    int nants;

    // The "SRC_NAME" FITS header.
    // The name of the current target.
    char src_name[81];

    // The "TELESCOP" FITS header.
    // The name of the telescope.
    char telescop[81];

    // The "HDR_SIZE" FITS header.
    // The size of header in bytes, not including directio padding.
    size_t hdr_size; 

    // NOTE: this comment is wrong and missing_blocks is wrong.
    // It should increase by piperblk, not by 1.
    // TODO: fix it
    // Normally, pktidx increases by 1 each block.
    // In some cases, the process writing the .raw file doesn't write
    // some of the blocks that it would normally. missing_blocks
    // provides the number of blocks that were "missing" in this way,
    // between this header and the previous block.
    // Depending on the downstream application, you might want to
    // treat these blocks as being equivalent to all zeros, or
    // simply ignore them.
    int missing_blocks;

    // The number of timesteps in the data.
    // This isn't stored explicitly in a FITS header because we can
    // calculate it from the other metadata, but it's useful so we include it here.
    int num_timesteps;

    // The number of frequency channels in the data.
    // These are typically "coarse channels" because we later convert into more channels.
    // This does *not* count different antennas multiple times. It
    // isn't stored explicitly in a FITS header for historical reasons
    // and is instead calculated from the other metadata.
    // In particular it is different from obsnchan.
    int num_channels;

    // Gets the unix start time of this block.
    // Calculates based on SYNCTIME and PIPERBLK headers. I think there is some
    // error because SYNCTIME is rounded to the nearest second, but the error is at
    // least consistent across blocks in a file.
    // Assert-fails if a required header is missing, so be careful.
    double getStartTime() {
      long synctime = getUnsignedInt("SYNCTIME", -1);
      assert(synctime > 0);
      long piperblk = getUnsignedInt("PIPERBLK", -1);
      assert(piperblk > 0);
      double time_per_packet = tbin * num_timesteps / piperblk;
      return synctime + pktidx * time_per_packet;
    }

    // Gets the unix time that represents the temporal midpoint of the block.
    double getMidTime() {
      return getStartTime() + (tbin * num_timesteps) / 2.0;
    }
    
    // Helper to parse an int32 from the header
    int getInt(const char* key, int default_value) {
      char tmpstr[48];
      int32_t value;
      if (libwcs::hgeti4(buffer, key, &value) == 0) {
        if (libwcs::hgets(buffer, key, 48, tmpstr) == 0) {
          value = default_value;
        } else {
          value = strtol(tmpstr, NULL, 0);
        }
      }
      return value;
    }

    // Helper to parse a uint32 from the header
    unsigned int getUnsignedInt(const char* key, uint32_t default_value) {
      char tmpstr[48];
      uint32_t value;
      if (libwcs::hgetu4(buffer, key, &value) == 0) {
        if (libwcs::hgets(buffer, key, 48, tmpstr) == 0) {
          value = default_value;
        } else {
          value = strtoul(tmpstr, NULL, 0);
        }
      }
      return value;
    }

    unsigned long getUnsignedLong(const char* key, unsigned long default_value) {
      char tmpstr[48];
      uint64_t value;
      if (libwcs::hgetu8(buffer, key, &value) == 0) {
        if (libwcs::hgets(buffer, key, 48, tmpstr) == 0) {
          value = default_value;
        } else {
          value = strtoull(tmpstr, NULL, 0);
        }
      }
      return value;
    }

    double getDouble(const char* key, double default_value) {
      char tmpstr[48];
      double value;
      if (libwcs::hgetr8(buffer, key, &value) == 0) {
        if (libwcs::hgets(buffer, key, 48, tmpstr) == 0) {
          value = default_value;
        } else {
          value = strtod(tmpstr, NULL);
        }
      }
      return value;
    }

  };

}
