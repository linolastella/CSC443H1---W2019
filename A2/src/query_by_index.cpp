/*
 * Lino Lastella 1001237654 lastell1
 *
 * Program 3: query_by_index.cpp
 *
 * Make a query and print the results found.
 *
 * Compiled with GCC 7.4.0
 */


#include <fstream>
#include <array>
#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include "query_by_index.hpp"


// Global variables declaration
FieldType inputfield;
size_t numbuckets;
size_t pagesize;
size_t idxpagesread;
size_t datapagesread;
unsigned short entrysize;


unsigned long long get_last_n_bits (const unsigned int n, const std::string &key)
{
    auto h_key = std::hash<std::string> {} (key);
    return h_key & ((1 << n) - 1);
}


IndexType get_type_of_index (const std::string &idx_file_name)
{
    std::ifstream idx_file (idx_file_name, std::ios::binary);
    if (idx_file.is_open ())
    {
        idx_file.seekg (0);
        std::array<char, 1> idx_type;

        idx_file.read (idx_type.data (), idx_type.size ());
        if (idx_file.good ())
        {
            switch (idx_type[0])
            {
                case '0': return IndexType::STATIC;
                case '1': return IndexType::EXTENDIBLE;
                case '2': return IndexType::LINEAR_HASHING;
            }
        }
        else
        {
            std::cerr << "Something went wrong when reading index type" << std::endl;
        }
    }
    else
    {
        std::cerr << "Could not open index file for reading" << std::endl;
    }

    return IndexType (2);  // Returning invalid index on purpose
}


std::string get_val_from_bin (std::vector<char> bin_val)
{
    // Remove all ASCII(0)s
    bin_val.erase (std::remove (bin_val.begin (), bin_val.end (), '\0'), bin_val.end ());

    // Turn into string
    return std::string (bin_val.cbegin (), bin_val.cend ());
}


void find_rids_in_page (const std::vector<char> &page, std::list<size_t> &rowids, const std::string &value)
{
    for (size_t i = 0; i < (pagesize - PTR_SIZE) / entrysize; ++i)
    {
        auto key_start = std::next (page.cbegin (), i * entrysize);
        auto key = get_val_from_bin (std::vector<char> (key_start, key_start + entrysize - RID_SIZE));
        if (key == value)
        {
            auto key_rid = get_val_from_bin (std::vector<char> (key_start + entrysize - RID_SIZE,
                                                                key_start + entrysize));
            rowids.push_back (std::stoul (key_rid, nullptr, BASE_HEX));
        }
    }
}


bool read_header (std::ifstream &idx_file)
{
    idx_file.seekg (1, std::ios::beg);

    std::vector<char> field_bin (1), page_size_bin (4), num_buckets_bin (4);
    idx_file.read (&field_bin[0], 1).read (&page_size_bin[0], 4).read (&num_buckets_bin[0], 4);

    if (!idx_file.good ())
    {
        std::cerr << "Could not read index file header" << std::endl;
        return false;
    }

    if (inputfield != FieldType (std::stoi (get_val_from_bin (field_bin))))
    {
        std::cerr << "Field from index file and user input don't match. Are you trying to trick me??" << std::endl;
        return false;
    }

    pagesize   = std::stoul (get_val_from_bin (page_size_bin), nullptr, BASE_HEX);
    numbuckets = std::stoul (get_val_from_bin (num_buckets_bin), nullptr, BASE_HEX);

    // Reading the header counts as 1 page read
    idxpagesread++;

    return true;
}


std::vector<char> get_static_primary_bucket (std::ifstream &idx_file, const std::string &value)
{
    std::vector<char> prim_page (pagesize - PTR_SIZE);

    // Get primary bucket number for queried value and seek there
    auto bucket_num = std::hash<std::string> {} (value) % numbuckets;

    idx_file.seekg (bucket_num * pagesize, std::ios::cur);

    // Load primary page in memory
    idx_file.read (&prim_page[0], pagesize - PTR_SIZE);
    if (!idx_file.good ())
    {
        std::cerr << "Could not read primary bucket in memory" << std::endl;
    }

    // One more index page has been read
    idxpagesread++;

    return prim_page;
}


std::list<size_t> get_rids_static_index (const std::string &idx_file_name, const std::string &value)
{
    std::list<size_t> rids;

    std::ifstream idx_file (idx_file_name, std::ios::binary);
    if (!idx_file.is_open ())
    {
        std::cerr << "Could not open index file for reading" << std::endl;
        return rids;
    }

    if (!read_header (idx_file))
    {
        return rids;
    }

    std::vector<char> primary_bucket = get_static_primary_bucket (idx_file, value);
    if (primary_bucket.empty ())
    {
        return rids;
    }

    find_rids_in_page (primary_bucket, rids, value);

    // Get pointer to start of overflow pages
    std::vector<char> overflow_ptr_bin (PTR_SIZE);
    idx_file.read (&overflow_ptr_bin[0], PTR_SIZE);

    // No need to increment the total number of index pages read since the pointer is within the same page as the
    // primary bucket page

    if (!idx_file.good ())
    {
        std::cerr << "Could not read pointer to overflow pages" << std::endl;
        return rids;
    }

    // Read block of sequential overflow pages (if any) until there are no more
    read_sequential_overflow_pages (idx_file, overflow_ptr_bin, rids, value);

    return rids;
}


void init_entrysize ()
{
    switch (inputfield)
    {
        case FieldType::FIRST_NAME: entrysize = FIRST_NAME_MAX_LEN + RID_SIZE; break;
        case FieldType::LAST_NAME:  entrysize = LAST_NAME_MAX_LEN  + RID_SIZE; break;
        case FieldType::EMAIL:      entrysize = EMAIL_MAX_LEN      + RID_SIZE; break;
    }
}


void print_record (const std::array<char, RECORD_SIZE> &record)
{
    auto first_name_start = record.cbegin ();
    auto last_name_start  = first_name_start + FIRST_NAME_MAX_LEN;
    auto email_start      = last_name_start  + LAST_NAME_MAX_LEN;

    std::cout << "First name: " << std::string (first_name_start, last_name_start) << '\n'
              << "Last name : " << std::string (last_name_start, email_start)      << '\n'
              << "Email: "      << std::string (email_start, record.cend ())       << std::endl;
}


void print_matching_records (const std::string &db_file_name, const std::list<size_t> &row_ids)
{
    if (!row_ids.empty ())
    {
        std::ifstream db_file (db_file_name, std::ios::binary);
        if (db_file.is_open ())
        {
            for (auto this_rid : row_ids)
            {
                db_file.seekg (this_rid * RECORD_SIZE, std::ios::beg);

                std::array<char, RECORD_SIZE> this_record;
                db_file.read (this_record.data (), this_record.size ());

                if (db_file.good ())
                {
                    // We assume that every row is in a different page
                    datapagesread++;

                    print_record (this_record);
                    std::cout << std::endl;
                }
                else
                {
                    std::cerr << "Could not read record in db binary file" << std::endl;
                }
            }

            std::cout << "Total number of matching record: " << row_ids.size () << std::endl;
        }
        else
        {
            std::cerr << "Could not open db file for retrieving records" << std::endl;
        }
    }
    else
    {
        std::cerr << "No records were found matching input value" << std::endl;
    }
}


int read_header_and_bits_required (std::ifstream &idx_file)
{
    if (!read_header (idx_file))
    {
        return 0;
    }

    // Reading two more bits does not imply reading one whole additional index page, so no incrementing done here

    std::vector<char> num_bits_bin (2);
    idx_file.read (&num_bits_bin[0], 2);

    if (!idx_file.good ())
    {
        std::cerr << "Could not read number of bits for hash value in linear hashing" << std::endl;
        return 0;
    }

    return std::stoi (get_val_from_bin (num_bits_bin), nullptr, BASE_HEX);
}


std::vector<char> get_linear_primary_bucket (std::ifstream &idx_file, const std::string &value, int num_bits)
{
    std::vector<char> prim_page (pagesize - PTR_SIZE);

    // Get primary bucket number for queried value and seek there
    auto bucket_num = get_last_n_bits (num_bits, value);

    idx_file.seekg (bucket_num * pagesize, std::ios::cur);

    // Load primary page in memory
    idx_file.read (&prim_page[0], pagesize - PTR_SIZE);
    if (!idx_file.good ())
    {
        std::cerr << "Could not read primary bucket in memory" << std::endl;
    }

    // One more index page has been read
    idxpagesread++;

    return prim_page;
}


std::list<size_t> get_rids_linear_index (const std::string &idx_file_name, const std::string &value)
{
    std::list<size_t> rids;

    std::ifstream idx_file (idx_file_name, std::ios::binary);
    if (!idx_file.is_open ())
    {
        std::cerr << "Could not open index file for reading" << std::endl;
        return rids;
    }

    auto num_bits = read_header_and_bits_required (idx_file);
    if (num_bits == 0)
    {
        return rids;
    }

    std::vector<char> primary_bucket = get_linear_primary_bucket (idx_file, value, num_bits);
    if (primary_bucket.empty ())
    {
        return rids;
    }

    find_rids_in_page (primary_bucket, rids, value);

    // Get pointer to start of overflow pages
    std::vector<char> overflow_ptr_bin (PTR_SIZE);
    idx_file.read (&overflow_ptr_bin[0], PTR_SIZE);

    if (!idx_file.good ())
    {
        std::cerr << "Could not read pointer to overflow pages" << std::endl;
        return rids;
    }

    // Read block of sequential overflow pages (if any) until there are no more
    read_sequential_overflow_pages (idx_file, overflow_ptr_bin, rids, value);

    return rids;
}


void read_sequential_overflow_pages (std::ifstream &idx_file, const std::vector<char> &overflow_ptr_bin,
                                     std::list<size_t> &rids, const std::string &value)
{
    std::streamoff overflow_ptr = std::stoll (get_val_from_bin (overflow_ptr_bin), nullptr, BASE_HEX);
    while (overflow_ptr != 0)
    {
        idx_file.seekg (overflow_ptr, std::ios::beg);

        std::vector<char> cur_of_page (pagesize - PTR_SIZE);
        idx_file.read (&cur_of_page[0], pagesize - PTR_SIZE);

        if (!idx_file.good ())
        {
            std::cerr << "Could not read overflow page" << std::endl;
        }

        // Just read one more index page
        idxpagesread++;

        find_rids_in_page (cur_of_page, rids, value);

        std::vector<char> next_ptr_bin (PTR_SIZE);
        idx_file.read (&next_ptr_bin[0], PTR_SIZE);

        if (!idx_file.good ())
        {
            std::cerr << "Could not ready end of overflow page" << std::endl;
        }

        auto next_ptr (get_val_from_bin (next_ptr_bin));
        if (next_ptr.empty ())
        {
            overflow_ptr = idx_file.tellg ();
        }
        else
        {
            overflow_ptr = std::stoul (next_ptr, nullptr, BASE_HEX);
        }
    }
}


std::vector<char> get_extendible_primary_bucket (std::ifstream &idx_file, const std::string &value, int num_bits)
{
    // Read the directory to find the primary bucket number
    auto directory_idx = get_last_n_bits (num_bits, value);
    idx_file.seekg (directory_idx * 4, std::ios::cur);
    std::vector<char> bucket_num_bin (4);

    idx_file.read (&bucket_num_bin[0], 4);
    if (!idx_file.good ())
    {
        std::cerr << "Could not read primary bucket number in memory" << std::endl;
    }

    idxpagesread++;

    // Get primary bucket number for queried value and seek there
    auto bucket_num = std::stoi (get_val_from_bin (bucket_num_bin), nullptr, BASE_HEX);

    // Seek to common header + extendible hashing header + entire directory
    const std::streamoff to_seek_to = 10 + 2 + 4 * (1 << num_bits) + bucket_num * pagesize;
    idx_file.seekg (to_seek_to, std::ios::beg);

    std::vector<char> prim_page (pagesize - PTR_SIZE);

    // Load primary page in memory
    idx_file.read (&prim_page[0], pagesize - PTR_SIZE);
    if (!idx_file.good ())
    {
        std::cerr << "Could not read primary bucket in memory" << std::endl;
    }

    idxpagesread++;

    return prim_page;
}


std::list<size_t> get_rids_extendible_index (const std::string &idx_file_name, const std::string &value)
{
    std::list<size_t> rids;

    std::ifstream idx_file (idx_file_name, std::ios::binary);
    if (!idx_file.is_open ())
    {
        std::cerr << "Could not open index file for reading" << std::endl;
        return rids;
    }

    auto num_bits = read_header_and_bits_required (idx_file);
    if (num_bits == 0)
    {
        return rids;
    }

    auto primary_bucket = get_extendible_primary_bucket (idx_file, value, num_bits);
    if (primary_bucket.empty ())
    {
        return rids;
    }

    find_rids_in_page (primary_bucket, rids, value);

    // Get pointer to start of overflow pages
    std::vector<char> overflow_ptr_bin (PTR_SIZE);
    idx_file.read (&overflow_ptr_bin[0], PTR_SIZE);

    if (!idx_file.good ())
    {
        std::cerr << "Could not read pointer to overflow pages" << std::endl;
        return rids;
    }

    // Read block of sequential overflow pages (if any) until there are no more
    read_sequential_overflow_pages (idx_file, overflow_ptr_bin, rids, value);

    return rids;
}


int main (int argc, char const *argv[])
{
    const std::string program_name (argv[0]);
    const std::string exe_name (program_name.substr (program_name.rfind ('/') + 1) + ".exe");

    if (argc -1 != NUM_REQUIRED_INPUTS)
    {
        std::cerr << "Usage: "
                  << exe_name
                  << " <.db input file name>"
                  << " <index file name created previously>"
                  << " <the index of the field to be sorted by; 0=First Name, 1=Last Name, 2=Email>"
                  << " <value to be searched, field = value>"
                  << std::endl;

        return 1;
    }

    std::string db_file_name (argv[1]);
    std::string idx_file_name (argv[2]);
    inputfield    = FieldType (std::stoi (argv[3]));
    auto idx_type = get_type_of_index (idx_file_name);
    std::string queried_value (argv[4]);

    if (inputfield != FieldType::FIRST_NAME && inputfield != FieldType::LAST_NAME && inputfield != FieldType::EMAIL)
    {
        std::cerr << "field can only be one of {0 - First Name, 1 - Last Name, 2 - Email}" << std::endl;
        return 1;
    }
    if (idx_type != IndexType::STATIC && idx_type != IndexType::EXTENDIBLE && idx_type != IndexType::LINEAR_HASHING)
    {
        std::cerr << "Invalid index type. Must be one of {0 - Static, 1 - Extendible, 2 - Linear hashing" << std::endl;
        return 1;
    }

    init_entrysize ();

    std::list<size_t> row_ids;
    switch (idx_type)
    {
        case IndexType::STATIC:         row_ids = get_rids_static_index (idx_file_name, queried_value);     break;
        case IndexType::EXTENDIBLE:     row_ids = get_rids_extendible_index (idx_file_name, queried_value); break;
        case IndexType::LINEAR_HASHING: row_ids = get_rids_linear_index (idx_file_name, queried_value);     break;
    }

    print_matching_records (db_file_name, row_ids);

    std::cout << "Total number of index pages read: " << idxpagesread  << '\n'
              << "Total number of data pages read: "  << datapagesread << std::endl;

    return 0;
}
