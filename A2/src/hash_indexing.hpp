/*
 * Lino Lastella 1001237654 lastell1
 *
 * Program 2: hash_indexing.hpp
 *
 * Header file
 */


#pragma once

#include <vector>
#include <list>
#include <cassert>
#include <numeric>
#include "common_constants.hpp"


// Constants

constexpr unsigned short MIN_NUM_BUCKETS     = 2;
constexpr unsigned short NUM_REQUIRED_INPUTS = 6;
constexpr unsigned short RID_SIZE            = 5;
constexpr unsigned short PTR_SIZE            = 6;


// Type aliases and enumerated types

enum class IndexType : unsigned short
{
    STATIC,
    EXTENDIBLE,
    LINEAR_HASHING
};


// Global namespace classes declarations

struct DataEntry;
class Page;
class Bucket;
class Root;


// Global variables

extern FieldType field;
extern IndexType typeofindex;


// Functions declarations

void initialize (const char *argv[]);
bool check_initialization ();

void write_all_entries_in_page (std::ofstream &output_file, const Page &this_page);
void write_all_of_pages_in_bucket (std::ofstream &output_file, const Bucket &bucket, std::streampos &bytes_counter);
void write_index_file (const char *file_name, const Root *main_obj);

void print_results (const Root *main_obj);
void populate (const char *input_file_name, Root *main_obj);

unsigned long long get_last_n_bits (const unsigned int n, const std::string &key);
bool kth_bit_is_on (const unsigned long long target, const unsigned int k);


// Classes

/**
 * DataEntry struct 
 * 
 * Holds a key and a row id
 */
struct DataEntry
{
    std::string key;
    unsigned int rid;

    static unsigned short size;

    DataEntry (const std::string &this_key, const unsigned int this_rid) :
        key (this_key), rid (this_rid) {}

    DataEntry (const DataEntry &data_entry) :
        DataEntry (data_entry.key, data_entry.rid) {}

    // Define two <DataEntry>s to be equal if they share the same key (rows will never be equal)
    inline bool operator== (const DataEntry &other) const
    {
        return other.key == this->key;
    }
};

/**
 * Page class
 * 
 * Holds a collection of data entries
 */
class Page
{
    std::vector<DataEntry> data_entries;

public:

    Page ()
    {
        data_entries.reserve ((page_size - PTR_SIZE) / DataEntry::size);
    }

    Page (const DataEntry &data_entry) :
        Page ()
    {
        data_entries.push_back (data_entry);
    }

    // Define two <Page>s to be equal if their data_entries are equal
    inline bool operator== (const Page &other) const
    {
        return other.data_entries == this->data_entries;
    }

    // The following two functions are needed for the range-based for loop
    inline auto begin () const { return data_entries.begin (); }
    inline auto end () const { return data_entries.end (); }

    inline void add_entry (const DataEntry &data_entry)
    {
        data_entries.emplace_back (data_entry);
    }

    // Page fits more entries if the number of data entries in it plus 1 times the size of a DataEntry
    // is less than or equal to <page_size> - <PTR_SIZE>
    inline bool fits_more_entries () const
    {
        return (data_entries.size () + 1) * DataEntry::size <= page_size - PTR_SIZE;
    }

    // Return true if all data entries in this page share the same key (or the page is empty)
    inline bool all_duplicates () const
    {
        return !is_empty () &&
            std::count (data_entries.cbegin (), data_entries.cend (), first_entry ()) == data_entries.size ();
    }

    // Get first data entry in this page
    inline const DataEntry &first_entry () const
    {
        return data_entries.front ();
    }

    // Return true if there are no data entries in this page
    inline bool is_empty () const
    {
        return data_entries.empty ();
    }

    // Return true if the last <n> bits of the hashed value of <new_key> are different than
    // the last <n> bits of the hashed value of at least one key of any data entry
    inline bool key_different_of_any (const std::string &new_key, const unsigned short n) const
    {        
        return
            std::any_of (data_entries.cbegin (), data_entries.cend (),
                [&] (const auto &this_entry)
                {
                    return get_last_n_bits (n, new_key) != get_last_n_bits (n, this_entry.key);
                });
    }

    void remove_entries_start_with_1 (const unsigned int num_bits);
    void remove_entries_start_with_0 (const unsigned int num_bits);

    inline void rearrange_entries (Page &new_page, const unsigned int num_bits)
    {
        // Remove data entries that start with 1 from this page and data entries that start with 0 from new page
        this->remove_entries_start_with_1 (num_bits);
        new_page.remove_entries_start_with_0 (num_bits);
    }

    // Get last data entry in this page
    inline const DataEntry &last_entry () const
    {
        return data_entries.back ();
    }

    // Remove the last data entry in this page
    inline void pop ()
    {
        data_entries.pop_back ();
    }

    static size_t page_size;
};

/**
 * Bucket class
 * 
 * Holds a linked list of pages
 * 
 */
class Bucket
{
protected:

    unsigned int my_overflow_pages;
    std::list<Page> pages_list;

public:

    Bucket (const unsigned int num_overflow_pages, const std::list<Page> &existing_pages_list) :
        my_overflow_pages (num_overflow_pages), pages_list (existing_pages_list) {}

    Bucket () :
        Bucket (0, std::list<Page> (1)) {}  // Initialize <pages_list> to contain one empty Page

    // The following two functions are needed for the iteration
    inline auto begin () const { return pages_list.begin (); }
    inline auto end () const { return pages_list.end (); }

    // Insert entry in first page with enough space (or create a new overflow page if they are all full)
    void add_entry (const DataEntry &data_entry);

    inline const Page &get_primary_page () const
    {
        return pages_list.front ();
    }

    // A Bucket is considered to be empty if it contains only an empty Page
    inline bool is_empty () const
    {
        return get_primary_page ().is_empty ();
    }

    // Return true if primary page fits at least one more data entry
    inline bool fits_more_entries () const
    {
        return get_primary_page ().fits_more_entries ();
    }

    inline const DataEntry &first_entry () const
    {
        return get_primary_page ().first_entry ();
    }

    inline bool contains_overflow_pages () const
    {
        return my_overflow_pages > 0;
    }

    inline unsigned int get_num_overflow_pages () const
    {
        return my_overflow_pages;
    }

    // Return true if all the pages in this bucket contain duplicates
    inline bool all_duplicates () const
    {
        return std::all_of (pages_list.cbegin (), pages_list.cend (), [] (const Page &cur_page)
        {
            return cur_page.all_duplicates ();
        });
    }

    void squeeze_my_pages ();

    void rearrange_entries (Bucket &new_bucket, const unsigned int num_bits);
};

/**
 * Root class
 * 
 * This is an abstract class
 */
class Root
{
protected:

    // This structure maps number of pages to number of buckets holding that many pages
    // (used for the histogram)
    std::vector<unsigned long long> histogram_vec;

public:

    virtual ~Root () = default;

    virtual void add_entry (const DataEntry &data_entry) = 0;

    virtual unsigned long long get_num_buckets () const = 0;
    virtual size_t get_num_idx_pages ()           const = 0;
    virtual size_t get_total_overflow_pages ()    const = 0;
    virtual void populate_histogram_vec () = 0;

    virtual const Bucket &get_bucket (const unsigned long long bucket_num) const = 0;

    virtual unsigned short bits_to_look () const
    {
        // This function is not pure virtual because StaticHashing::Index does not override it
        return 0;
    }

    virtual const std::vector<unsigned short> &get_directory () const
    {
        // This function is not pure virtual because only ExtendibleHashing overrides it
        return {};
    }

    virtual unsigned short get_bin (const unsigned short bin_num) const = 0;
};

namespace StaticHashing
{
    /**
     * Index class
     * 
     * Holds a vector of buckets
     * 
     */
    class Index : public Root
    {
        std::vector<Bucket> my_buckets;

    public:

        Index (const size_t static_num_buckets) :
            my_buckets (static_num_buckets) {}

        inline void add_entry (const DataEntry &data_entry) override
        {
            my_buckets.at (std::hash<std::string> {} (data_entry.key) % get_num_buckets ()).add_entry (data_entry);
        }

        inline unsigned long long get_num_buckets () const override
        {
            return my_buckets.size ();
        }

        inline size_t get_num_idx_pages () const override
        {
            return get_num_buckets ();
        }

        inline const Bucket &get_bucket (const unsigned long long bucket_num) const override
        {
            return my_buckets.at (bucket_num);
        }

        inline size_t get_total_overflow_pages () const override
        {
            size_t num_of_pages = 0;
            std::for_each (my_buckets.cbegin (), my_buckets.cend (),
                [&] (const auto &this_bucket) { num_of_pages += this_bucket.get_num_overflow_pages (); });
            return num_of_pages;
        }

        void populate_histogram_vec () override;
        unsigned short get_bin (const unsigned short bin_num) const override
        {
            return std::accumulate (histogram_vec.cbegin () + (bin_num - 1) * 4,
                                    histogram_vec.cbegin () + bin_num * 4, 0);
        }
    };
} // StaticHashing

namespace ExtendibleHashing
{
    /**
     * ExtBucket class
     * 
     * Bucket for extendible hashing, slightly different than regular (global namespace) Buckets
     */
    class ExtBucket : public Bucket
    {
        unsigned short local_depth;

    public:

        ExtBucket (unsigned short depth) :
            Bucket (), local_depth (depth) {}

        inline void rearrange_entries (ExtBucket &new_bucket)
        {
            pages_list.front ().rearrange_entries (new_bucket.pages_list.front (), local_depth);
        }

        // Return true if the last <local_depth> bits of the hashed value of <new_key>
        // are different than the last <local_depth> bits at least one hashed value of any key in this bucket
        inline bool key_different_of_any (const std::string &new_key) const
        {
            return
                std::any_of (pages_list.cbegin (), pages_list.cend (),
                    [&] (const auto &this_page)
                    {
                        return this_page.key_different_of_any (new_key, local_depth);
                    });
        }

        inline void increment_local_depth ()
        {
            local_depth++;
        }

        inline unsigned short get_local_depth () const
        {
            return local_depth;
        }
    };

    /**
     * Index class
     * 
     * Holds a vector of ExtBucket objects, a directory (vector of numbers) and a global depth counter
     */
    class Index : public Root
    {
        unsigned short global_depth;
        std::vector<ExtBucket> my_buckets;
        std::vector<unsigned short> directory;

        inline ExtBucket &get_bucket_from_dir_idx (const unsigned long long directory_idx)
        {
            return my_buckets.at (directory.at (directory_idx));
        }

    public:

        Index (const unsigned short depth) :
            global_depth (depth), my_buckets (1 << depth, depth), directory (1 << depth)
        {
            std::iota (directory.begin (), directory.end (), 0);
        }

        void add_entry (const DataEntry &data_entry) override;

        inline unsigned long long get_num_buckets () const override
        {
            return directory.size ();
        }

        inline size_t get_num_idx_pages () const override
        {
            return my_buckets.size ();
        }

        inline const Bucket &get_bucket (const unsigned long long bucket_idx) const override
        {
            return my_buckets.at (bucket_idx);
        }

        inline size_t get_total_overflow_pages () const override
        {
            size_t num_of_pages = 0;
            std::for_each (my_buckets.cbegin (), my_buckets.cend (),
                [&] (const auto &this_bucket) { num_of_pages += this_bucket.get_num_overflow_pages (); });
            return num_of_pages;
        }

        inline unsigned short bits_to_look () const override
        {
            return global_depth;
        }

        inline const std::vector<unsigned short> &get_directory () const override
        {
            return directory;
        }

        void populate_histogram_vec () override;
        unsigned short get_bin (const unsigned short bin_num) const override
        {
            return std::accumulate (histogram_vec.cbegin () + (bin_num - 1) * 4,
                                    histogram_vec.cbegin () + bin_num * 4, 0);
        }
    };
} // ExtendibleHashing

namespace LinearHashing
{
    /**
     * Index class
     * 
     * Holds a vector of Bucket objects, a directory (vector of numbers) and a few counters
     */
    class Index : public Root
    {
        unsigned int level = 0;
        size_t next = 0;

        const unsigned int init_num_buckets;
        size_t num_buckets_round_start;

        std::vector<Bucket> my_buckets;

        inline Bucket &get_bucket (const unsigned long long idx)
        {
            return my_buckets.at (idx);
        }

    public:
        Index (const size_t init_num_buckets):
            init_num_buckets (init_num_buckets), num_buckets_round_start (init_num_buckets),
            my_buckets (init_num_buckets) {}

        void add_entry (const DataEntry &data_entry) override;

        inline unsigned long long get_num_buckets () const override
        {
            return my_buckets.size ();
        }

        inline size_t get_num_idx_pages () const override
        {
            return get_num_buckets ();
        }

        inline const Bucket &get_bucket (const unsigned long long idx) const override
        {
            return my_buckets.at (idx);
        }

        inline size_t get_total_overflow_pages () const override
        {
            size_t num_of_pages = 0;
            std::for_each (my_buckets.cbegin (), my_buckets.cend (),
                [&] (const auto &this_bucket) { num_of_pages += this_bucket.get_num_overflow_pages (); });
            return num_of_pages;
        }

        inline unsigned short bits_to_look () const override
        {
            return level + static_cast<unsigned int> (std::log2 (init_num_buckets));
        }

        void populate_histogram_vec () override;
        unsigned short get_bin (const unsigned short bin_num) const override
        {
            return std::accumulate (histogram_vec.cbegin () + bin_num - 1,
                                    histogram_vec.cbegin () + bin_num, 0);
        }
    };
} // LinearHashing
