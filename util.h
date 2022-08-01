#ifndef __RAW_UTIL_H
#define __RAW_UTIL_H

#include "hget.h"

// Utilities ported from plain C.

namespace raw {
  // Reads `bytes_to_read` bytes from `fd` into the buffer pointed to by `buf`.
  // Returns the total bytes read or -1 on error.  A non-negative return value
  // will be less than `bytes_to_read` only of EOF is reached.
  inline ssize_t read_fully(int fd, char* buf, size_t bytes_to_read) {
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

  inline ssize_t pread_fully(int fd, char* buf, size_t bytes_to_read, off_t offset) {
    ssize_t bytes_read;
    ssize_t total_bytes_read = 0;
    while (bytes_to_read > 0) {
      bytes_read = pread(fd, buf, bytes_to_read, offset);
      if (bytes_read == 0) {
        break;
      }
      if (bytes_read < 0) {
        return -1;
      }

      buf += bytes_read;
      offset += bytes_read;
      bytes_to_read -= bytes_read;
      total_bytes_read += bytes_read;
    }

    return total_bytes_read;
  }
  
  inline void rawspec_raw_get_str(const char * buf, const char * key, const char * def,
			   char * out, size_t len)
  {
    if (libwcs::hgets(buf, key, len, out) == 0) {
      strncpy(out, def, len);
      out[len-1] = '\0';
    }
  }

  inline double rawspec_raw_dmsstr_to_d(char * dmsstr)
  {
    int sign = 1;
    double d = 0.0;

    char * saveptr;
    char * tok;

    if(dmsstr[0] == '-') {
      sign = -1;
      dmsstr++;
    } else if(dmsstr[0] == '+') {
      dmsstr++;
    }

    tok = strtok_r(dmsstr, ":", &saveptr);
    if(tok) {
      // Degrees (or hours)
      d = strtod(tok, NULL);

      tok = strtok_r(NULL, ":", &saveptr);
      if(tok) {
	// Minutes
	d += strtod(tok, NULL) / 60.0;

	tok = strtok_r(NULL, ":", &saveptr);
	if(tok) {
	  // Seconds
	  d += strtod(tok, NULL) / 3600.0;
	  tok = strtok_r(NULL, ":", &saveptr);
	}
      }
    } else {
      d = strtod(dmsstr, NULL);
    }

    return sign * d;
  }

  inline int rawspec_raw_header_size(char * hdr, int len, int directio)
  {
    int i;

    // Loop over the 80-byte records
    for(i=0; i<len; i += 80) {
      // If we found the "END " record
      if(!strncmp(hdr+i, "END ", 4)) {
	//printf("header_size: found END at record %d\n", i);
	// Move to just after END record
	i += 80;
	// Move past any direct I/O padding
	if(directio) {
	  i += (MAX_RAW_HEADER_SIZE - i) % 512;
	}
	return i;
      }
    }
    return 0;
  }
  
  // Parses rawspec related RAW header params from buf into header.
  inline void rawspec_raw_parse_header(Header* header) {
    int smjd;
    int imjd;
    char tmp[80];

    header->blocsize = header->getInt("BLOCSIZE", 0);
    header->npol     = header->getInt("NPOL", 0);
    header->obsnchan = header->getInt("OBSNCHAN", 0);
    header->nbits    = header->getUnsignedInt("NBITS", 8);
    header->obsfreq  = header->getDouble("OBSFREQ", 0.0);
    header->obsbw    = header->getDouble("OBSBW", 0.0);
    header->tbin     = header->getDouble("TBIN", 0.0);
    header->directio = header->getInt("DIRECTIO", 0);
    header->pktidx   = header->getUnsignedLong("PKTIDX", -1);
    header->beam_id  = header->getInt("BEAM_ID", -1);
    header->nants    = header->getUnsignedInt("NANTS", 1);

    rawspec_raw_get_str(header->buffer, "RA_STR", "0.0", tmp, 80);
    header->ra = rawspec_raw_dmsstr_to_d(tmp);

    rawspec_raw_get_str(header->buffer, "DEC_STR", "0.0", tmp, 80);
    header->dec = rawspec_raw_dmsstr_to_d(tmp);

    imjd = header->getInt("STT_IMJD", 51545);
    smjd = header->getInt("STT_SMJD", 0);
    header->mjd = ((double)imjd) + ((double)smjd)/86400.0;

    rawspec_raw_get_str(header->buffer, "SRC_NAME", "Unknown", header->src_name, 80);
    rawspec_raw_get_str(header->buffer, "TELESCOP", "Unknown", header->telescop, 80);
  }
  
  // Reads RAW file params from fd.  On entry, fd is assumed to be at the start
  // of a RAW header section.  On success, this function returns the file offset
  // of the subsequent data block and the file descriptor `fd` will also refer to
  // that location in the file.  On EOF, this function returns 0.  On failure,
  // this function returns -1 and the location to which fd refers is undefined.
  inline off_t rawspec_raw_read_header(int fd, Header* raw_hdr) {
    int hdr_size;
    off_t pos = lseek(fd, 0, SEEK_CUR);

    // Read header (plus some data, probably)
    hdr_size = read(fd, raw_hdr->buffer, MAX_RAW_HEADER_SIZE);

    if(hdr_size == -1) {
      return -1;
    } else if(hdr_size < 80) {
      return 0;
    }

    rawspec_raw_parse_header(raw_hdr);

    if(raw_hdr->blocsize ==  0) {
      fprintf(stderr, "BLOCSIZE not found in header\n");
      return -1;
    }
    if(raw_hdr->npol  ==  0) {
      fprintf(stderr, "NPOL not found in header\n");
      return -1;
    }
    if(raw_hdr->obsnchan ==  0) {
      fprintf(stderr, "OBSNCHAN not found in header\n");
      return -1;
    }
    if(raw_hdr->obsfreq  ==  0.0) {
      fprintf(stderr, "OBSFREQ not found in header\n");
      return -1;
    }
    if(raw_hdr->obsbw    ==  0.0) {
      fprintf(stderr, "OBSBW not found in header\n");
      return -1;
    }
    if(raw_hdr->tbin     ==  0.0) {
      fprintf(stderr, "TBIN not found in header\n");
      return -1;
    }
    if(raw_hdr->pktidx   == -1) {
      fprintf(stderr, "PKTIDX not found in header\n");
      return -1;
    }

    // TODO: Figure out why we do this.
    // 4 is the number of possible cross pol products
    if(raw_hdr->npol == 4) {
      // 2 is the actual number of polarizations present
      raw_hdr->npol = 2;
    }

    // Save the header size with no padding
    raw_hdr->hdr_size = rawspec_raw_header_size(raw_hdr->buffer, hdr_size, 0);

    // Get size of header plus padding
    hdr_size = rawspec_raw_header_size(raw_hdr->buffer, hdr_size, raw_hdr->directio);
    //printf("RRP: hdr=%lu\n", hdr_size);

    // Seek forward from original position past header (and any padding)
    pos = lseek(fd, pos + hdr_size, SEEK_SET);
    //printf("RRP: seek=%ld\n", pos);

    return pos;
  }  

}

#endif
