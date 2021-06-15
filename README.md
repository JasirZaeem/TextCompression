# Text Compression

This is a command line driven c++ program that uses a greedy approach using Huffman Coding algorithm to compress text based, and any binary file that has an uneven distribution of the possible 8 bit values in it.

## Build

`g++ main.cpp`

## Usage

### To compress a file

`./huffman [input file name] [output file name]`
`./huffman -c/--compress [input file name] [output file name]`

### To decompress a file

`./huffman -d/--decompress [input file name] [output file name]`

### To show help

`./huffman -h/--help`