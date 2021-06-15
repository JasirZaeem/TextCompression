#include <bitset>
#include <climits>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <queue>
#include <string>

typedef unsigned char Byte;

// Constants

const char PADDING_BIT = '0';
const char LEFT_CHAR = '0';
const char RIGHT_CHAR = '1';
const char NULL_CHAR = '\0';
std::string COMPRESSED_FILE_EXTENSION;

// Structs

struct HuffmanNode {
  Byte byte;
  uint32_t freq;
  HuffmanNode *left, *right;

  HuffmanNode() = default;

  HuffmanNode(Byte _byte, uint32_t _freq, HuffmanNode *_left = nullptr,
              HuffmanNode *_right = nullptr)
      : byte{_byte}, freq{_freq}, left{_left}, right{_right} {}

  HuffmanNode(HuffmanNode *_left, HuffmanNode *_right)
      : byte{NULL_CHAR}, freq{_left->freq + _right->freq}, left{_left},
        right{_right} {}
};

struct greater_frequency {
  bool operator()(HuffmanNode *l, HuffmanNode *r) { return l->freq > r->freq; }
};

// Utils

std::streampos get_file_size(std::ifstream &file);
std::string byte_to_bit_string(Byte byte);
std::string bytes_to_bit_string(const std::vector<Byte> &bytes);

// UI

void show_help(bool intended = false);
void handle_args(int argc, char **argv);
void file_compressed_message(uint32_t data_size, uint32_t compressed_size,
                             const char *filename);

// Huffman Algorithm

std::map<Byte, uint32_t> count_frequencies(const std::vector<Byte> &data);
HuffmanNode *build_huffman_tree(const std::map<Byte, uint32_t> &frequencies);
void create_substitution_table(HuffmanNode *huffman_tree_root,
                               std::map<Byte, std::string> &substitution_table,
                               const std::string &substitute_str = "");
void decode(HuffmanNode *root, int &index, const std::string &str,
            std::vector<Byte> &decoded);

// File IO

std::vector<Byte> read_uncompressed_file(const char *filename);
uint32_t read_compressed_file(const char *filename, std::vector<Byte> &data,
                              std::map<Byte, uint32_t> &frequencies);
void write_uncompressed_file(const std::vector<Byte> &data,
                             const char *filename);
void write_compressed_file(uint32_t original_file_size,
                           std::string &compressed_data, uint32_t padding,
                           const std::map<Byte, uint32_t> &frequencies,
                           const char *filename);

// Compression

uint32_t compress(const std::vector<Byte> &data, const char *filename);
void compress_to_file(const char *from_file, const char *_to_file);

// Decompression

std::vector<Byte> decompress(std::string &bits, uint32_t padding,
                             const std::map<Byte, uint32_t> &frequencies);
void decompress_to_file(const char *from_file, const char *to_file);

// Main

int main(int argc, char **argv) {
  COMPRESSED_FILE_EXTENSION = std::string(".huff");
  handle_args(argc, argv);
  return 0;
}

// Functions Definitions

// Utils

std::streampos get_file_size(std::ifstream &file) {
  file.seekg(0, std::ios::end);
  std::streampos file_size = file.tellg();
  file.seekg(0, std::ios::beg);
  return file_size;
}

std::string byte_to_bit_string(Byte byte) {
  return std::bitset<CHAR_BIT>(byte).to_string();
}

std::string bytes_to_bit_string(const std::vector<Byte> &bytes) {
  std::string bits;
  bits.reserve(bytes.size() * CHAR_BIT);

  for (auto byte : bytes) {
    bits += byte_to_bit_string(byte);
  }

  return bits;
}

// UI

void show_help(bool intended) {

  std::cout << std::string(40, '=');

  if (!intended) {
    std::cout << "Missing or invalid arguments" << std::endl;
  }

  std::cout << "Following commands are available" << std::endl;

  std::cout << "To compress a file" << std::endl << std::endl;

  std::cout << "./huffman [input file name] [output file name]" << std::endl;
  std::cout << "./huffman -c/--compress [input file name] [output file name]"
            << std::endl
            << std::endl;

  std::cout << "To decompress a file" << std::endl;
  std::cout << "./huffman -d/--decompress [input file name] [output file name]"
            << std::endl
            << std::endl;

  std::cout << "To show this help" << std::endl;
  std::cout << "./huffman -h/--help" << std::endl;
}

void handle_args(int argc, char **argv) {
  switch (argc) {
  case 2: {
    if (argv[1] == std::string("-h") || argv[1] == std::string("--help")) {
      show_help(true);
    }
    break;
  }

  case 3: {
    compress_to_file(argv[1], argv[2]);
    break;
  }

  case 4: {
    if (argv[1] == std::string("-d") ||
        argv[1] == std::string("--decompress")) {

      decompress_to_file(argv[2], argv[3]);

    } else if (argv[1] == std::string("-c") ||
               argv[1] == std::string("--compress")) {

      compress_to_file(argv[2], argv[3]);

    } else {
      show_help();
    }
    break;
  }

  default: {
    show_help();
    break;
  }
  }
}

void file_compressed_message(uint32_t data_size, uint32_t compressed_size,
                             const char *filename) {
  if (compressed_size < data_size) {
    double percentage_reduction =
        double(data_size - compressed_size) / data_size * 100;
    std::cout << filename << " was compressed from " << data_size
              << " bytes, to " << compressed_size << " bytes.\nSaving "
              << percentage_reduction << "% space";
  }
}

// Huffman Algorithm

std::map<Byte, uint32_t> count_frequencies(const std::vector<Byte> &data) {
  std::map<Byte, uint32_t> frequencies;

  for (auto byte : data) {
    frequencies[byte]++;
  }

  return frequencies;
}

void create_substitution_table(HuffmanNode *huffman_tree_root,
                               std::map<Byte, std::string> &substitution_table,
                               const std::string &substitute_str) {
  if (!huffman_tree_root)
    return;

  // found a leaf node
  if (!huffman_tree_root->left && !huffman_tree_root->right) {
    substitution_table[huffman_tree_root->byte] = substitute_str;
  }

  create_substitution_table(huffman_tree_root->left, substitution_table,
                            substitute_str + LEFT_CHAR);
  create_substitution_table(huffman_tree_root->right, substitution_table,
                            substitute_str + RIGHT_CHAR);
}

HuffmanNode *build_huffman_tree(const std::map<Byte, uint32_t> &frequencies) {
  std::priority_queue<HuffmanNode *, std::vector<HuffmanNode *>,
                      greater_frequency>
      nodeHeap;

  for (auto pair : frequencies) {
    nodeHeap.push(new HuffmanNode(pair.first, pair.second));
  }

  while (nodeHeap.size() > 1) {
    HuffmanNode *left = nodeHeap.top();
    nodeHeap.pop();
    HuffmanNode *right = nodeHeap.top();
    nodeHeap.pop();
    nodeHeap.push(new HuffmanNode(left, right));
  }

  return nodeHeap.top();
}

void decode(HuffmanNode *root, int &index, const std::string &str,
            std::vector<Byte> &decoded) {
  if (!root)
    return;

  if (!root->left && !root->right) {
    decoded.push_back(root->byte);
    return;
  }

  ++index;

  if (str[index] == LEFT_CHAR)
    decode(root->left, index, str, decoded);
  else
    decode(root->right, index, str, decoded);
}

// File IO

std::vector<Byte> read_uncompressed_file(const char *filename) {
  std::ifstream output_file(filename, std::ios::binary);

  output_file.unsetf(std::ios::skipws);

  std::streampos file_size = get_file_size(output_file);

  std::vector<Byte> vec;
  vec.reserve(file_size);

  vec.insert(vec.begin(), std::istream_iterator<Byte>(output_file),
             std::istream_iterator<Byte>());

  return vec;
}

uint32_t read_compressed_file(const char *filename, std::vector<Byte> &data,
                              std::map<Byte, uint32_t> &frequencies) {
  std::ifstream input_file(filename, std::ios::binary);
  input_file.unsetf(std::ios::skipws);

  // Read header

  uint32_t original_file_size;
  input_file.read(reinterpret_cast<char *>(&original_file_size),
                  sizeof(original_file_size));

  uint32_t data_size;
  input_file.read(reinterpret_cast<char *>(&data_size), sizeof(data_size));

  uint32_t padding;
  input_file.read(reinterpret_cast<char *>(&padding), sizeof(padding));

  uint32_t frequencies_size;
  input_file.read(reinterpret_cast<char *>(&frequencies_size),
                  sizeof(frequencies_size));

  for (int i = 0; i < frequencies_size; ++i) {
    Byte ch;
    uint32_t frequency;
    input_file.read(reinterpret_cast<char *>(&ch), sizeof(ch));
    input_file.read(reinterpret_cast<char *>(&frequency), sizeof(frequency));
    frequencies[ch] = frequency;
  }

  data = std::vector<Byte>(data_size);
  input_file.read(reinterpret_cast<char *>(reinterpret_cast<char *>(&data[0])),
                  static_cast<int>(sizeof(Byte) * data_size));
  input_file.close();

  return padding;
}

void write_uncompressed_file(const std::vector<Byte> &data,
                             const char *filename) {
  std::ofstream output_file(filename, std::ios::trunc | std::ios::binary);
  output_file.write(reinterpret_cast<const char *>(&data[0]),
                    static_cast<int>(sizeof(Byte) * data.size()));
  output_file.close();
}

void write_compressed_file(uint32_t original_file_size,
                           std::string &compressed_data, uint32_t padding,
                           const std::map<Byte, uint32_t> &frequencies,
                           const char *filename) {

  std::ofstream output_file(filename, std::ios::trunc | std::ios::binary);

  // Pad data

  compressed_data += std::string(padding, PADDING_BIT);

  // Write Header

  auto data_size = compressed_data.length() / CHAR_BIT;
  auto frequencies_size = frequencies.size();

  output_file.write(reinterpret_cast<const char *>(&original_file_size),
                    sizeof(original_file_size));
  output_file.write(reinterpret_cast<const char *>(&data_size),
                    sizeof(data_size));
  output_file.write(reinterpret_cast<const char *>(&padding), sizeof(padding));
  output_file.write(reinterpret_cast<const char *>(&frequencies_size),
                    sizeof(frequencies_size));

  // Write frequency table

  for (auto pair : frequencies) {
    output_file.write(reinterpret_cast<const char *>(&pair.first),
                      sizeof(pair.first));
    output_file.write(reinterpret_cast<const char *>(&pair.second),
                      sizeof(pair.second));
  }

  // Write compressed data

  for (int i = 0; i < compressed_data.length(); i += CHAR_BIT) {
    auto byte = static_cast<char>(
        std::bitset<CHAR_BIT>(compressed_data, i, CHAR_BIT).to_ulong() &
        0xFFul);
    output_file.write(reinterpret_cast<const char *>(&byte), sizeof(byte));
  }

  output_file.close();
}

// Compression

uint32_t compress(const std::vector<Byte> &data, const char *filename) {

  uint32_t original_file_size = data.size();
  auto frequencies = count_frequencies(data);
  HuffmanNode *root = build_huffman_tree(frequencies);
  std::map<Byte, std::string> substitution_table;
  create_substitution_table(root, substitution_table);

  uint32_t encodedSize = 0;
  for (const auto &pair : substitution_table) {
    encodedSize += frequencies[pair.first] * pair.second.length();
  }

  uint32_t padding = CHAR_BIT - (encodedSize % CHAR_BIT);
  std::string encoded;
  encoded.reserve(encodedSize + padding);
  for (Byte ch : data) {
    encoded += substitution_table[ch];
  }

  write_compressed_file(original_file_size, encoded, padding, frequencies,
                        filename);

  return encoded.length() / CHAR_BIT;
}

void compress_to_file(const char *from_file, const char *_to_file) {
  auto data = read_uncompressed_file(from_file);

  std::string to_file(_to_file);

  auto end =
      to_file.length() > 5 ? to_file.substr(to_file.length() - 5) : to_file;

  if (end != COMPRESSED_FILE_EXTENSION)
    to_file += COMPRESSED_FILE_EXTENSION;

  uint32_t compressed_size = compress(data, to_file.c_str());
  file_compressed_message(data.size(), compressed_size, from_file);
}

// Decompression

std::vector<Byte> decompress(std::string &bits, uint32_t padding,
                             const std::map<Byte, uint32_t> &frequencies) {
  HuffmanNode *root = build_huffman_tree(frequencies);

  bits.erase(bits.length() - padding);
  int index = -1;
  std::vector<Byte> decoded;
  while (index < static_cast<int>(bits.size()) - 2) {
    decode(root, index, bits, decoded);
  }

  return decoded;
}

void decompress_to_file(const char *from_file, const char *to_file) {
  std::vector<Byte> data;
  std::map<Byte, uint32_t> frequencies;

  uint32_t padding = read_compressed_file(from_file, data, frequencies);

  std::string bits = bytes_to_bit_string(data);
  std::vector<Byte> decompressed = decompress(bits, padding, frequencies);

  write_uncompressed_file(decompressed, to_file);
}
