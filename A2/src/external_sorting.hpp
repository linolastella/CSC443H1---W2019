/**
 * Lino Lastella 1001237654 lastell1
 *
 * Program 1: external_sorting.hpp
 *
 * Header file
 */

#pragma once

#include <vector>
#include <set>
#include "common_constants.hpp"


// Constants

constexpr auto MIN_NUM_BUFFERS     = 3;
constexpr auto NUM_REQUIRED_INPUTS = 5;
constexpr auto TEMP_OUTPUT         = "temp_output.bin";


// Type aliases and enumerated types

// tuple of <first name> <last name> <email>
using InfoTuple = std::array<std::string, 3>;

// data structure to hold a record
using RecordType = std::array<char, RECORD_SIZE>;

// data structure to store a buffer page (collection of records)
using BufferType = std::vector<RecordType>;

// data structure to store a collection of buffer pages
using BuffersType = std::vector<BufferType>;

// data structure to store a collection of offsets
using PositionsType = std::vector<std::streamoff>;

// multiset of info_tuples
// Elements inserted will be automatically sorted according to the default operator< of tuples
using MySpecialMultiset = std::multiset<InfoTuple>;

enum
{
    NO_OFFSET = -1
};


// Global variables

// These counters will be incremented at every page read/write
extern unsigned long totalpagesread, totalpageswritten, totalpasses;

// These ones will be initialized after reading input and never changed
extern float numbuffers;
extern size_t pagesize;
extern std::string inputfilename;
extern std::string outputfilename;


// Functions declarations
InfoTuple extract_info (const RecordType &this_record);
bool      populate_set (const BufferType &this_buffer, MySpecialMultiset &pages_set);

void Pass0_write (MySpecialMultiset &pages_set, std::ofstream &output_file, const size_t pages_processed);
bool Pass0_read  (MySpecialMultiset &pages_set, std::ifstream &input_file,  const size_t pages_to_process,
                       unsigned long long &pages_left);

std::array<unsigned long long, 2> Pass0_full ();

BufferType read_records_in_page  (std::ifstream &input_file, const std::streamoff offset);
bool       write_records_in_page (std::ofstream &output_file, const BufferType &buffer);

size_t find_correct_buffer_idx  (const BuffersType &buffers);
bool   initialize_input_buffers (BuffersType &buffers, const std::string &sorted_file_name,
                                     PositionsType &starting_pos_this_run);

size_t merge_this_run (PositionsType &starting_pos_this_run, const PositionsType &ending_pos_this_run,
                           BufferType &output_buffer, BuffersType &buffers, const std::string &cur_sorted_file_name,
                           const std::string &writable_file_name);
size_t write_leftover (BufferType &output_buffer, BuffersType &buffers,
                           const std::string &writable_file_name, size_t pages_written_on_merge);

bool Pass1_till_end (const unsigned long long num_pages);
