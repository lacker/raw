# raw
A header-only C++ library for handling the GUPPI Raw Data Format.

Developed for C++ 11, with no other runtime dependencies. For testing you need CMake 3.10 or better.

## The Raw Data Format

The raw data format isn't as standardized as one would like. The goal of this code is to be compatible
with the raw file processing done in [rawspec](https://github.com/UCBerkeleySETI/rawspec) and [hashpipe](https://github.com/MydonSolutions/hashpipe). Whenever it's unclear what the right thing to do is, look there to see how they do it.

For a detailed documentation of the format that this library handles, the best reference is the code for the
`raw::Header` object [here](https://github.com/lacker/raw/blob/master/header.h).

## Usage

The easiest way to include this library is as a git submodule. It is header-only, so including the `raw/raw.h` header
will get you everything you need.

The raw data format alternates between headers and data blocks.
The typical workflow is creating a `raw::Reader` object to read a file, then `readHeader` to read the next header, and `readData`
to read the data block. For example, here is some sample code that will call `handleData` on every block of data in a raw file:

```
raw::Reader reader(filename);
raw::Header header;
while (reader.readHeader(&header)) {
  vector<char> data(header.blocsize);
  reader.readData(data.data());
  handleData(data);
}

if (reader.error()) {
   cerr << "error: " << reader.errorMessage() << endl;
}
```

This data is typically a multidimensional array; see the [comments](https://github.com/lacker/raw/blob/master/header.h)
in `raw::Header` for more information.

## Testing

To run the tests:

```
./run_tests.sh
```

This requires a particular large data file which is currently only available at the Berkeley datacenter. Sorry for the inconvenience.