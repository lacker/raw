#pragma once

namespace raw {

  typedef struct {
    int directio;
    size_t blocsize;
    unsigned int npol;
    unsigned int obsnchan;
    unsigned int nbits;
    int64_t pktidx; // TODO make uint64_t?
    double obsfreq;
    double obsbw;
    double tbin;
    double ra;  // hours
    double dec; // degrees
    double mjd;
    int beam_id; // -1 is unknown or single beam receiver
    int nbeam;   // -1 is unknown or single beam receiver
    unsigned int nants;
    char src_name[81];
    char telescop[81];
    off_t hdr_pos; // Offset of start of header
    size_t hdr_size; // Size of header in bytes (not including DIRECTIO padding)
  } Header;

}
