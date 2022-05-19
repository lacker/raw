#pragma once

namespace raw {

  /*
    The Header contains the information we get from processing one block of the .raw file.

   The .raw file format is similar to the FITS format:
     https://fits.gsfc.nasa.gov/fits_primer.html
  
   However, the header does not include the fields that are required in FITS, and it does not store the data blocks
   in any of the ways that are supported by the FITS format, so you generally cannot expect to use libraries designed to read FITS
   files on a .raw file. The headers are formatted the same way, though, minus the missing required fields, so we can use some code
   originally written to parse FITS headers.
   
   Most of the fields of the Header map directly to FITS headers. Some of them are auxiliary information
   generated during the parsing process.
  */
  typedef struct {
    int directio;

    // The "BLOCSIZE" FITS header.
    // This is the size of the following data segment, in bytes, not including directio padding.
    size_t blocsize;

    // The "NPOL" FITS header, with the exception that any 4 is treated like a 2, for reasons that are lost in time.
    // This is the number of polarities for the data.
    unsigned int npol;

    // The "OBSNCHAN" FITS header.
    // This is the number of frequency channels for the data.
    unsigned int obsnchan;

    // The "NBITS" FITS header.
    // This is the number of bits used to store each real or complex values, as a signed int.
    // We only support nbits = 8.
    unsigned int nbits;

    // The "PKTIDX" FITS header.
    // This is an index that counts up through the file, to let us detect missing blocks.
    int64_t pktidx;

    // The "OBSFREQ" FITS header.
    // This is the center frequency of the entire range of frequencies stored in the file.
    // TODO: what units is this in?
    double obsfreq;

    // The "OBSBW" FITS header.
    // This is the width of the range of frequencies. Negative indicates a reversed frequency axis.
    // TODO: what units is this in?
    double obsbw;

    // The "TBIN" FITS header.
    // This is the time resolution for the data.
    // TODO: what units is this in?
    double tbin;

    // The right ascension of the telescope.
    // TODO: what units - hours? degrees?
    // This is derived from the "RA_STR" FITS header.
    double ra;

    // The declination of the telescope, in degrees.
    // TODO: what units - degrees?
    // This is derived from the "DEC_STR" FITS header.
    double dec;

    // The start time in MJD format.
    // This is synthesized from the "STT_IMJD" and "STT_SMJD" FITS headers.
    double mjd;

    // The "BEAM_ID" FITS header.
    // The beam id. -1 is unknown or a single beam receiver.
    int beam_id;

    // The "NBEAM" FITS header.
    // The number of total beams. -1 is unknown or a single beam receiver.
    int nbeam;

    // The "NANTS" FITS header.
    // This is the number of antennas in the data.
    // TODO: how do antennas relate to the data layout?
    unsigned int nants;

    // The "SRC_NAME" FITS header.
    // The name of the current target.
    char src_name[81];

    // The "TELESCOP" FITS header.
    // The name of the telescope.
    char telescop[81];

    // The position of the start of the header in the .raw file.    
    off_t hdr_pos;

    // The "HDR_SIZE" FITS header.
    // The size of header in bytes, not including directio padding.
    size_t hdr_size; 

    // Normally, pktidx increases by 1 each block.
    // In some cases, the process writing the .raw file doesn't write some of the blocks that it would normally.
    // missing_blocks provides the number of blocks that were "missing" in this way, between this header and the previous block.
    // Depending on the downstream application, you might want to treat these blocks as being equivalent to all zeros, or
    // simply ignore them.
    int missing_blocks;
  } Header;

}
