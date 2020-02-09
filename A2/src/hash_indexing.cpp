/*
 * Lino Lastella 1001237654 lastell1
 *
 * Program 2: hash_indexing.cpp
 *
 * Create a hash-based index file using different hashing techniques.
 *
 * Compiled with GCC 7.4.0
 * 
 * Note to grader: I used the same naming practices as in external_sorting.cpp here as well.
 *  Additionally, I did not feel comfortable using external source code for the MD5 algorithm, and since I could
 *  not find any built-in functionality in C++ that reflects what the handout is asking, I decided to stick with the
 *  std::hash function object from the <functional> library to get hashed values for data keys
 */


#include <string>
#include <iostream>
#include <fstream>
#include <array>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include "hash_indexing.hpp"


// Global variables declaration

FieldType field;
IndexType typeofindex;


// Global namespace functions

void initialize (const char *argv[])
{
    typeofindex     = IndexType (std::stoi (argv[3]));
    field           = FieldType (std::stoi (argv[6]));
    Page::page_size = std::stoul (argv[5]);

    switch (field)
    {
        case FieldType::FIRST_NAME: DataEntry::size = FIRST_NAME_MAX_LEN + RID_SIZE; break;
        case FieldType::LAST_NAME:  DataEntry::size = LAST_NAME_MAX_LEN  + RID_SIZE; break;
        case FieldType::EMAIL:      DataEntry::size = EMAIL_MAX_LEN      + RID_SIZE; break;
    }
}


bool check_initialization ()
{
    if (field != FieldType::FIRST_NAME &&
        field != FieldType::LAST_NAME  &&
        field != FieldType::EMAIL)
    {
        std::cerr << "field can only be one of {0 - First Name, 1 - Last Name, 2 - Email}" << std::endl;
        return false;
    }
    if (typeofindex != IndexType::STATIC     &&
        typeofindex != IndexType::EXTENDIBLE &&
        typeofindex != IndexType::LINEAR_HASHING)
    {
        std::cerr << "index must be one of {0 - Static, 1 - Extendible, 2 - Linear hashing}" << std::endl;
        return false;
    }
    if (Page::page_size < MIN_PAGE_SIZE || Page::page_size % MIN_PAGE_SIZE != 0)
    {
        std::cerr << "Data page size must be a positive multiple of " << MIN_PAGE_SIZE << std::endl;
        return false;
    }

    return true;
}


void print_results (const Root *main_obj)
{
    std::cout << "Final number of buckets: "  << main_obj->get_num_buckets ()           << '\n'
              << "Number of index pages: "    << main_obj->get_num_idx_pages ()         << '\n'
              << "Number of overflow pages: " << main_obj->get_total_overflow_pages ()  << '\n'
              << "Distribution of pages: "                                              << '\n'
              << "Number of buckets containing 0-3 pages: "   << main_obj->get_bin (1)  << '\n'
              << "Number of buckets containing 4-7 pages: "   << main_obj->get_bin (2)  << '\n'
              << "Number of buckets containing 8-11 pages: "  << main_obj->get_bin (3)  << '\n'
              << "Number of buckets containing 12-15 pages: " << main_obj->get_bin (4)  << '\n'
              << "Number of buckets containing 16-19 pages: " << main_obj->get_bin (5)  << '\n'
              << "Number of buckets containing 20-23 pages: " << main_obj->get_bin (6)  << '\n'
              << "Number of buckets containing 24-27 pages: " << main_obj->get_bin (7)  << '\n'
              << "Number of buckets containing 28-31 pages: " << main_obj->get_bin (8)  << '\n'
              << "Number of buckets containing 32-35 pages: " << main_obj->get_bin (9)  << '\n'
              << "Number of buckets containing 36+   pages: " << main_obj->get_bin (10) << std::endl;
}


void populate (const char *input_file_name, Root *main_obj)
{
    std::ifstream input_file (input_file_name, std::ios::binary | std::ios::ate);
    if (input_file.is_open ())
    {
        auto file_size = input_file.tellg ();
        input_file.seekg (0, std::ios::beg);
        std::array<char, RECORD_SIZE> current_record;

        for (unsigned int row = 0; row < file_size / RECORD_SIZE; ++row)
        {
            input_file.read (current_record.data (), current_record.size ());
            if (input_file.good ())
            {
                const auto rid = row;
                std::string key;

                const auto first_name_start (current_record.cbegin ());
                const auto last_name_start  (std::next (first_name_start, FIRST_NAME_MAX_LEN));
                const auto email_start (std::next (last_name_start, LAST_NAME_MAX_LEN));

                switch (field)
                {
                    case FieldType::FIRST_NAME:
                        key = std::string (first_name_start, std::find (first_name_start, last_name_start, '\0'));
                        break;

                    case FieldType::LAST_NAME:
                        key = std::string (last_name_start, std::find (last_name_start, email_start, '\0'));
                        break;

                    case FieldType::EMAIL:
                        key = std::string (email_start, std::find (email_start, current_record.cend (), '\0'));
                        break;
                }

                main_obj->add_entry (DataEntry (key, rid));
            }
            else
            {
                std::cerr << "Something went wrong when reading input file to populate index object" << std::endl;
            }
        }

        main_obj->populate_histogram_vec ();
    }
    else
    {
        std::cerr << "Could not open input file to populate index object" << std::endl;
    }
}


unsigned long long get_last_n_bits (const unsigned int n, const std::string &key)
{
    auto h_key = std::hash<std::string> {} (key);
    return h_key & ((1 << n) - 1);
}


// Kth as in English counting, not computer science counting (i.e. k >= 1)
bool kth_bit_is_on (const unsigned long long target, const unsigned int k)
{
    return target & (1 << (k - 1));
}


void write_all_entries_in_page (std::ofstream &output_file, const Page &this_page)
{
    for (const auto &this_entry : this_page)
    {
        output_file << std::setw (static_cast<std::streamsize> (DataEntry::size - RID_SIZE)) << this_entry.key;
        output_file << std::setw (static_cast<std::streamsize> (RID_SIZE))                   << this_entry.rid;
    }
}


void write_all_of_pages_in_bucket (std::ofstream &output_file, const Bucket &this_bucket, std::streampos &bytes_counter)
{
    for (const auto &of_page : this_bucket)
    {
        // Skip primary page (it's not an overflow page)
        if (of_page == this_bucket.get_primary_page ()) continue;

        // Write empty page block
        auto this_page_beg = bytes_counter;
        output_file << std::string (Page::page_size, '\0');
        bytes_counter += Page::page_size;

        // Write content of overflow page
        output_file.seekp (this_page_beg);
        write_all_entries_in_page (output_file, of_page);

        // Seek to end of page
        output_file.seekp (bytes_counter, std::ios::beg);
    }
}


void write_index_file (const char *file_name, const Root *main_obj)
{
    std::ofstream output_file (file_name, std::ios::binary);
    if (output_file.is_open ())
    {
        output_file << std::hex << std::uppercase << std::setfill ('\0');

        // First byte is always the type of index
        output_file << static_cast<short> (typeofindex);

        // Second byte is field
        output_file << static_cast<short> (field);

        // Then four bytes representing the page size
        output_file << std::setw (4) << Page::page_size;

        // Then four bytes representing the number of primary index pages
        auto num_idx_pages = main_obj->get_num_idx_pages ();
        output_file << std::setw (4) << num_idx_pages;

        // Store <bits_to_look> here
        auto bits_to_look = main_obj->bits_to_look ();
 
        auto bytes_counter = output_file.tellp ();
        auto data_start = 1 + 1 + 4 + 4;

        switch (typeofindex)
        {
            case IndexType::STATIC: /* Nothing special happens for static hashing */ break;
            case IndexType::EXTENDIBLE:

                // Insert two bytes representing global depth
                output_file << std::setw (2) << bits_to_look;

                // Write directory
                for (auto bucket_num : main_obj->get_directory ())
                {
                    output_file << std::setw (4) << bucket_num;
                }

                data_start += 2 + main_obj->get_num_buckets () * 4;

                break;

            case IndexType::LINEAR_HASHING:

                // For linear hashing insert only 2 more bytes in the header representing the number of bits
                // needed to extract the bucket number from the hash of the key
                output_file << std::setw (2) << bits_to_look;
                data_start += 2;

               break;
        }

        // Allocate a whole block for sequential primary buckets.
        // Then fill them one by one appending overflow pages after the block when necessary
        output_file << std::setw (static_cast<std::streamsize> (num_idx_pages * Page::page_size)) << '\0';
        bytes_counter = output_file.tellp ();

        for (size_t bucket_num = 0; bucket_num < num_idx_pages; ++bucket_num)
        {
            const auto &this_bucket = main_obj->get_bucket (bucket_num);
            if (!this_bucket.is_empty ())
            {
                auto prim_page_start_pos = data_start + Page::page_size * bucket_num;
                output_file.seekp (static_cast<std::streamoff> (prim_page_start_pos), std::ios::beg);

                // Write primary page data entries
                write_all_entries_in_page (output_file, this_bucket.get_primary_page ());

                // Seek to end of page - <PTR_SIZE>
                output_file.seekp (
                    static_cast<std::streamoff> (prim_page_start_pos + Page::page_size - PTR_SIZE),
                        std::ios::beg);

                if (!this_bucket.contains_overflow_pages ())
                {
                    // A pointer to location 0 means no more overflow pages
                    output_file << std::string (PTR_SIZE, '0');
                }
                else
                {
                    // Set first pointer
                    output_file << std::setw (PTR_SIZE) << bytes_counter;

                    // Seek there and start writing oveflow pages sequentially
                    output_file.seekp (bytes_counter, std::ios::beg);

                    // Write all overflow pages
                    write_all_of_pages_in_bucket (output_file, this_bucket, bytes_counter);

                    // Seek to end of last page - <PTR_SIZE>
                    output_file.seekp (bytes_counter - static_cast<std::streampos> (PTR_SIZE), std::ios::beg);

                    // Write four 0s to represent end of overflow pages
                    output_file << std::string (PTR_SIZE, '0');
                }
            }
        }
    }
    else
    {
        std::cerr << "Could not open index file for writing" << std::endl;
    }
}


/**
 * Class declarations/definitions
 */

unsigned short DataEntry::size;
size_t Page::page_size;


void Page::remove_entries_start_with_1 (const unsigned int num_bits)
{
    const auto starts_with_1 = [num_bits] (const DataEntry &data_entry) -> bool
    {
        return kth_bit_is_on (get_last_n_bits (num_bits, data_entry.key), num_bits);
    };

    // Remove data entries that start with 1 using the famous erase/remove C++ idiom
    data_entries.erase (std::remove_if (data_entries.begin (), data_entries.end (),
                            starts_with_1),
                                data_entries.end ());
}


void Page::remove_entries_start_with_0 (const unsigned int num_bits)
{
    const auto starts_with_0 = [num_bits] (const DataEntry &data_entry) -> bool
    {
        return !kth_bit_is_on (get_last_n_bits (num_bits, data_entry.key), num_bits);
    };

    // Remove data entries that start with 0 using the famous erase/remove C++ idiom
    data_entries.erase (std::remove_if (data_entries.begin (), data_entries.end (),
                            starts_with_0),
                                data_entries.end ());
}


void Bucket::add_entry (const DataEntry &data_entry)
{
    for (auto &cur_page : pages_list)
    {
        if (cur_page.fits_more_entries ())
        {
            cur_page.add_entry (data_entry);
            return;
        }
    }

    pages_list.emplace_back (data_entry);
    my_overflow_pages++;
}


void Bucket::squeeze_my_pages ()
{
    auto host_page_it = pages_list.begin ();
    while (my_overflow_pages > 0)
    {
        // Find first available "host" page (page with enough space for more data entries)
        while (host_page_it != pages_list.end () && !host_page_it->fits_more_entries ())
        {
            ++host_page_it;
        }

        // If all the overflow pages are full or there is only one non-full overflow page, then we can't squeeze them
        if (host_page_it == pages_list.end () || host_page_it == std::prev (pages_list.end ()))
        {
            break;
        }

        auto first_squeezable_page_it = std::next (host_page_it);
        while (!first_squeezable_page_it->is_empty () && host_page_it->fits_more_entries ())
        {
            host_page_it->add_entry (first_squeezable_page_it->last_entry ());
            first_squeezable_page_it->pop ();
        }

        // If the overflow page is now empty, remove it from the bucket
        if (first_squeezable_page_it->is_empty ())
        {
            pages_list.erase (first_squeezable_page_it);
            my_overflow_pages--;
        }
    }
}


void Bucket::rearrange_entries (Bucket &new_bucket, const unsigned int num_bits)
{
    // This bucket and <new_bucket> are identical at this point.
    // Remove every data entry whose hashed value starts with 1 from every page in the old bucket,
    // and every data entry whose hashed value starts with 0 from every page in the new bucket.
    // Along the way, if some overflow page becomes empty, remove it

    auto page_it = this->pages_list.begin ();
    while (page_it != this->pages_list.end ())
    {
        page_it->remove_entries_start_with_1 (num_bits);
        
        // Check if it's overflow and empty
        if (page_it != this->pages_list.begin () && page_it->is_empty ())
        {
            page_it = this->pages_list.erase (page_it);
            this->my_overflow_pages--;
        }
        else
        {
            ++page_it;
        }
    }

    page_it = new_bucket.pages_list.begin ();
    while (page_it != new_bucket.pages_list.end ())
    {
        page_it->remove_entries_start_with_0 (num_bits);
        
        // Check if it's overflow and empty
        if (page_it != new_bucket.pages_list.begin () && page_it->is_empty ())
        {
            page_it = new_bucket.pages_list.erase (page_it);
            new_bucket.my_overflow_pages--;
        }
        else
        {
            ++page_it;
        }
    }

    // Now squeeze both buckets' pages to use the least amount of overflow pages possible
    this->squeeze_my_pages ();
    new_bucket.squeeze_my_pages ();
}


namespace StaticHashing
{
    void Index::populate_histogram_vec ()
    {
        histogram_vec.clear ();
        histogram_vec.resize (40);

        for (const auto &this_bucket : my_buckets)
        {
            auto num_pages = this_bucket.get_num_overflow_pages () + (this_bucket.is_empty () ? 0 : 1);
            histogram_vec.at (num_pages)++;
        }
    }
} // StaticHashing


namespace ExtendibleHashing
{
    void Index::populate_histogram_vec ()
    {
        histogram_vec.clear ();
        histogram_vec.resize (40);

        for (const auto &this_bucket : my_buckets)
        {
            auto num_pages = this_bucket.get_num_overflow_pages () + (this_bucket.is_empty () ? 0 : 1);
            histogram_vec.at (num_pages)++;
        }
    }


    void Index::add_entry (const DataEntry &data_entry)
    {
        auto directory_idx   = get_last_n_bits (global_depth, data_entry.key);
        auto &correct_bucket = get_bucket_from_dir_idx (directory_idx);

        // Add the new data entry to this bucket if it is not full or if global depth is more than 25.
        // Also add the new data entry if the bucket contains all duplicates and the new data entry shares the same
        // key with any of them.
        // Otherwise, proceed with the splitting algorithm
        if (correct_bucket.fits_more_entries () ||
            global_depth > 25 ||
            (correct_bucket.all_duplicates () && data_entry == correct_bucket.first_entry ()))
        {
            correct_bucket.add_entry (data_entry);
        }
        else
        {
            auto init_loc_depth = correct_bucket.get_local_depth ();

            // If global depth is equal to local depth, then double the directory and recurse
            if (global_depth == init_loc_depth)
            {
                global_depth++;

                const auto old_size = directory.size ();

                directory.resize (2 * old_size);
                std::copy_n (directory.cbegin (), old_size, std::next (directory.begin (), old_size));
            }
            else
            {
                correct_bucket.increment_local_depth ();

                // If the buckets can be differentiated now, then proceed with splitting.
                // Otherwise recurse
                if (correct_bucket.key_different_of_any (data_entry.key))
                {
                    // Append a 1 to the front of the old directory index
                    auto new_directory_idx = directory_idx | (1 << init_loc_depth);

                    // Link directory to end-of-bucket
                    directory.at (new_directory_idx) = my_buckets.size ();

                    // Create a new bucket identical to current bucket
                    /* This call may invalidate <correct_bucket> */
                    my_buckets.push_back (correct_bucket);

                    // Rearrange the elements by looking at one extra bit to differentiate them
                    get_bucket_from_dir_idx (directory_idx).rearrange_entries (my_buckets.back ());
                }
            }

            add_entry (data_entry);
        }
    }
} // ExtendibleHashing


namespace LinearHashing
{
    void Index::populate_histogram_vec ()
    {
        histogram_vec.clear ();
        histogram_vec.resize (20);

        for (const auto &this_bucket : my_buckets)
        {
            auto num_pages = this_bucket.get_num_overflow_pages () + (this_bucket.is_empty () ? 0 : 1);
            histogram_vec.at (num_pages)++;
        }
    }


    void Index::add_entry (const DataEntry &data_entry)
    {
        // The initial number of buckets (N) is always a power of 2 (required from handout).
        // Then, <h_i (key)> = last <d_i> bits of <h (key)> where <d_i> = <log_2 (N)> + <i>
        auto d_i = level + static_cast<unsigned int> (std::log2 (init_num_buckets));

        auto bucket_num = get_last_n_bits (d_i, data_entry.key);
        if (bucket_num < next)
        {
            // Then we have to look at <h_{i+1} (key)>
            bucket_num = get_last_n_bits (d_i + 1, data_entry.key);
        }

        auto &correct_bucket = get_bucket (bucket_num);
        auto of_pages_before_insert = correct_bucket.get_num_overflow_pages ();

        // Always add the new data entry, even if it creates an overflow page (they will be dealt with at some point)
        correct_bucket.add_entry (data_entry);

        // Trigger the splitting algorithm only if we did create an overflow page
        if (of_pages_before_insert < correct_bucket.get_num_overflow_pages ())
        {
            // Create a copy of <next>
            my_buckets.push_back (get_bucket (next));

            // Rearrange elements by looking at one extra bit
            get_bucket (next).rearrange_entries (my_buckets.back (), d_i + 1);

            // Move <next>
            next++;

            // Deal with end of round situation
            if (next == num_buckets_round_start)
            {
                next = 0;
                num_buckets_round_start *= 2;
                level++;
            }
        }
    }

} // LinearHashing


/**
 * End of class declarations/definitions
 */


/**
 * Plan of attack for Question2
 *  - Get user input and do some checking.
 *  - Use polymorphism to create an appropriate index object (the classes structure is explained in the report)
 *  - Populate the index object with data entries and write it to disk (the binary output file format is explained
 *    in the report as well).
 *  - Print required stats
 */
int main (int argc, char const *argv[])
{
    const std::string program_name (argv[0]);
    const std::string exe_name (program_name.substr (program_name.rfind ('/') + 1) + ".exe");

    if (argc -1 != NUM_REQUIRED_INPUTS)
    {
        std::cerr << "Usage: "
                  << exe_name
                  << " <.db input file name>"
                  << " <index file name>"
                  << " <type of hash index: 0=Static, 1=Extendible, 2=Linear hashing>"
                  << " <initial number of buckets, power of 2>"
                  << " <data page size, multiple of 64>"
                  << " <the field to be indexed; 0=First Name, 1=Last Name, 2=Email>"
                  << std::endl;

        return 1;
    }

    const auto init_num_buckets = std::stoul (argv[4]);
    if (init_num_buckets < MIN_NUM_BUCKETS ||
        (init_num_buckets & (init_num_buckets - 1)) != 0 /* this hack checks power of 2 */)
    {
        std::cerr << "Number of buckets must be a power of two" << std::endl;
        return 1;
    }

    // Initialize globals and other bookkeeping and do some checking
    initialize (argv);
    if (!check_initialization ()) return 1;

    std::cout << "Running: " << exe_name << " " << argv[1]
              << " "         << argv[2]  << " " << argv[3]
              << " "         << argv[4]  << " " << argv[5]
              << " "         << argv[6]  << std::endl;

    Root *main_obj   = nullptr;
    const auto depth = static_cast<unsigned short> (std::log2 (init_num_buckets));

    switch (typeofindex)
    {
        case IndexType::STATIC:         main_obj = new StaticHashing::Index (init_num_buckets); break;
        case IndexType::EXTENDIBLE:     main_obj = new ExtendibleHashing::Index (depth);        break;
        case IndexType::LINEAR_HASHING: main_obj = new LinearHashing::Index (init_num_buckets); break;
    }

    populate (argv[1], main_obj);
    write_index_file (argv[2], main_obj);
    print_results (main_obj);

    delete main_obj;
    main_obj = nullptr;

    return 0;
}
