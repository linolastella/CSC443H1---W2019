/*
 * Lino Lastella 1001237654 lastell1
 *
 * Program 3: query_by_index.hpp
 *
 * Header file
 */


#pragma once

#include "common_constants.hpp"


// Constants

constexpr short NUM_REQUIRED_INPUTS = 4;
constexpr short BASE_HEX            = 16;
constexpr short PTR_SIZE            = 6;
constexpr short RID_SIZE            = 5;


// Type aliases and enumerated types

enum class IndexType
{
    STATIC,
    EXTENDIBLE,
    LINEAR_HASHING
};


// Global variables

extern FieldType inputfield;
extern size_t pagesize;
extern size_t numbuckets;
extern unsigned short entrysize;
extern size_t idxpagesread;
extern size_t datapagesread;


// Functions declarations

void init_entrysize ();
IndexType get_type_of_index (const std::string &idx_file_name);
bool read_header (std::ifstream &idx_file);
unsigned long long get_last_n_bits (const unsigned int n, const std::string &key);

std::string get_val_from_bin (std::vector<char> bin_val);
void find_rids_in_page (const std::vector<char> &page, std::list<size_t> &rowids, const std::string &value);
void read_sequential_overflow_pages (std::ifstream &idx_file, const std::vector<char> &overflow_ptr_bin,
                                     std::list<size_t> &rids, const std::string &value);

void print_record (const std::array<char, RECORD_SIZE> &record);
void print_matching_records (const std::string &db_file_name, const std::list<size_t> &row_ids);

std::list<size_t> get_rids_static_index (const std::string &idx_file_name, const std::string &value);
std::list<size_t> get_rids_extendible_index (const std::string &idx_file_name, const std::string &value);
std::list<size_t> get_rids_linear_index (const std::string &idx_file_name, const std::string &value);

std::vector<char> get_static_primary_bucket (std::ifstream &idx_file, const std::string &value);
std::vector<char> get_extendible_primary_bucket (std::ifstream &idx_file, const std::string &value, int num_bits);
std::vector<char> get_linear_primary_bucket (std::ifstream &idx_file, const std::string &value, int num_bits);

int read_header_and_bits_required (std::ifstream &idx_file);
