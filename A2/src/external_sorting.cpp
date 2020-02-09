/**
 * Lino Lastella 1001237654 lastell1
 *
 * Program 1: external_sorting.cpp
 * 
 * Compiled with GCC 7.4.0
 * 
 * Apply the "External Sorting" algorithm from class to sort the file and create a new db file.
 * 
 * Note to grader:
 *   I tried to stick to 120 characters max for every line of code, give meaningful variable/function names
 *   and be as self-documented as possible throughout the entire assignment.
 *   Constants are in ALL_CAPS, global variables are in lowercasenoseparators,
 *   type aliases and classes are in CamelCase and local variables are in lower_case_with_underscores.
 *   I think it might be easier to get a grasp of what's happening by jumping to <main> and working backwards to
 *   every function call ( just a suggestion :) )
 */


#include <array>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <algorithm>
#include "external_sorting.hpp"


// Global variables declaration
float numbuffers;
size_t pagesize;
FieldType field;
std::string inputfilename, outputfilename;

unsigned long totalpagesread = 0, totalpageswritten = 0, totalpasses = 0;


/**
 * Arguments:
 *   this_record - array of chars to extract data from
 * 
 * Return:
 *   tuple of <first name> <last name> <email>
 */
InfoTuple extract_info (const RecordType &this_record)
{
    const auto first_name_left_bound (this_record.cbegin ());
    const auto last_name_left_bound (std::next (first_name_left_bound, FIRST_NAME_MAX_LEN));
    const auto email_left_bound (std::next (last_name_left_bound, LAST_NAME_MAX_LEN));

    const std::string first_name (first_name_left_bound, last_name_left_bound);
    const std::string last_name (last_name_left_bound, email_left_bound);
    const std::string email (email_left_bound, this_record.cend ());

    return {first_name, last_name, email};
}


/**
 * Arguments:
 *   this_buffer - buffer of chars to extract data from
 *     pages_set - multiset from field to sort by to pair of remaining two fields
 * 
 * Return:
 *   true on success, false on failure
 */
bool populate_set (const BufferType &this_buffer, MySpecialMultiset &pages_set)
{
    for (auto &record : this_buffer)
    {
        auto infos = extract_info (record);

        const auto &first_name = infos[0];
        const auto &last_name  = infos[1];
        const auto &email      = infos[2];

        switch (field)
        {
            case FieldType::FIRST_NAME: pages_set.insert ({first_name, last_name, email}); break;
            case FieldType::LAST_NAME:  pages_set.insert ({last_name, first_name, email}); break;
            case FieldType::EMAIL:      pages_set.insert ({email, first_name, last_name}); break;
        }
    }

    return true;
}


/**
 * Arguments:
 *         pages_set - multiset containing sorted records
 *       output_file - file stream to write to
 *   pages_processed - number of pages processed
 */
void Pass0_write (MySpecialMultiset &pages_set, std::ofstream &output_file, const size_t pages_processed)
{
    // Write to output file. Keep same format as input file, i.e. <first name> <last name> <email>
    for (const auto &el : pages_set)
    {
        switch (field)
        {
            case FieldType::FIRST_NAME: output_file << el[0] << el[1] << el[2]; break;
            case FieldType::LAST_NAME:  output_file << el[1] << el[0] << el[2]; break;
            case FieldType::EMAIL:      output_file << el[1] << el[2] << el[0]; break;
        }
    }

    // Successfully wrote full run to disk
    totalpageswritten += pages_processed;
}


/**
 * Arguments:
 *   input_file - file stream to read records from
 *       offset - offset to start reading records (-1 if not required)
 * 
 * Return:
 *   buffer of records
 */
BufferType read_records_in_page (std::ifstream &input_file, const std::streamoff offset)
{
    const auto records_per_page = pagesize / RECORD_SIZE;
    BufferType buffer_to_fill (records_per_page);

    if (offset != NO_OFFSET) input_file.seekg (offset, std::ios::beg);

    for (auto &record : buffer_to_fill)
    {
        input_file.read (record.data (), record.size ());
        if (!input_file.good ()) return BufferType ();
    }

    return buffer_to_fill;
}


/**
 * Arguments:
 *   output_file - file stream to write records to
 * 
 * Return:
 *   Whether writing was successfull
 */
bool write_records_in_page (std::ofstream &output_file, const BufferType &buffer)
{
    for (auto &record : buffer)
    {
        output_file.write (record.data (), record.size ());
        if (!output_file.good ()) return false;
    }

    return true;
}


/**
 * Arguments:
 *          pages_set - multiset containing sorted  records
 *         input_file - file stream to read from
 *   pages_to_process - number of pages processed
 *         pages_left - reference to counter of number of pages left to read
 * 
 * Return:
 *   flag representing if everything went well
 */
bool Pass0_read (MySpecialMultiset &pages_set, std::ifstream &input_file,
                 const size_t pages_to_process, unsigned long long &pages_left)
{
    for (unsigned int i = 0; i < pages_to_process; ++i)
    {
        BufferType temp_buffer (read_records_in_page (input_file, NO_OFFSET));
        
        if (temp_buffer.size () * RECORD_SIZE == pagesize)
        {
            totalpagesread++;
            pages_left--;

            bool set_filled_success = populate_set (temp_buffer, pages_set);
            if (!set_filled_success)
            {
                std::cerr << "Something went wrong when populating pages_set in Pass 0" << std::endl;
                return false;
            }
        }
        else
        {
            std::cerr << "Something went wrong when reading a page from input file. Only "
                      << input_file.gcount ()
                      << " characters have been read in this page";
            return false;
        }
    }

    return true;
}


/**
 * Return:
 *   pair of (total number of pages in the input file, total number of pages successfully read)
 */
std::array<unsigned long long, 2> Pass0_full ()
{
    // Perform ⌈<num_pages> / <numbuffers>⌉ sorted runs of at most <numbuffers> pages each

    std::ifstream input_file  (inputfilename,  std::ios::binary | std::ios::ate);
    std::ofstream output_file (outputfilename, std::ios::binary);

    if (input_file.is_open () && output_file.is_open ())
    {
        const unsigned long long num_pages = input_file.tellg () / pagesize;
        input_file.seekg (0, std::ios::beg);

        auto pages_left = num_pages;

        // Pass 0 ends when we processed every page
        for (unsigned int cur_page_num = 1; cur_page_num <= num_pages; ++cur_page_num)
        {
            const auto pages_to_process = pages_left < size_t (numbuffers) ? pages_left : size_t (numbuffers);

            // Insert every record in a multiset of InfoTuples
            // It will be automatically sorted according to a comparator function defined in the header
            // This multiset holds min(<pages_left>, <numbuffers>) * <pagesize> data
            // and it is equivalent to "B buffers"
            MySpecialMultiset pages_set;

            if (!Pass0_read (pages_set, input_file, pages_to_process, pages_left))
            {
                return {num_pages, num_pages - pages_left};
            }

            Pass0_write (pages_set, output_file, pages_to_process);
        }

        return {num_pages, num_pages - pages_left};
    }

    std::cerr << "Could not open input or output file. Passed in '"
              << inputfilename
              << "', and '"
              << outputfilename
              << "')."
              << std::endl;
    return {0, 0};
}


/**
 * Arguments:
 *   buffers - vector of buffers containing sorted pages of records
 * 
 * Return:
 *   index of correct buffer. I.e., buffer to remove a record from
 */
size_t find_correct_buffer_idx (const BuffersType &buffers)
{
    size_t idx = 0;

    if (buffers.empty ())
    {
        std::cerr << "Something terribly wrong happened" << std::endl;
    }
    else
    {
        auto min_buffer = std::min_element (buffers.cbegin (), buffers.cend (),
            [] (const auto &b1, const auto &b2)
            {
                // Binary predicate to check if first buffer is "less" than the second
                return b2.empty () || (!b1.empty () && extract_info (b1.front ()) < extract_info (b2.front ()));
            });

        idx = static_cast<size_t> (min_buffer - buffers.cbegin ());
    }

    return idx;
}


/**
 * Arguments:
 *                 buffers - buffers to initialize
 *        sorted_file_name - name of file to write data to
 *   starting_pos_this_run - vector of offsets representing starting point for reading
 * 
 * Return:
 *   flag representing if everything went well
 */
bool initialize_input_buffers (BuffersType &buffers, const std::string &sorted_file_name,
                               PositionsType &starting_pos_this_run)
{
    std::ifstream sorted_file (sorted_file_name, std::ios::binary);

    if (sorted_file.is_open ())
    {
        // For every sorted block...
        for (unsigned int i = 0; i < starting_pos_this_run.size (); ++i)
        {
            auto &this_starting_pos = starting_pos_this_run[i];
            BufferType buffer (read_records_in_page (sorted_file, this_starting_pos));

            if (buffer.size () * RECORD_SIZE == pagesize)
            {
                // Read successfull
                // Increment global counter, increment starting positions vector and append buffer to buffers
                totalpagesread++;
                this_starting_pos += pagesize;
                buffers.push_back (buffer);
            }
            else
            {
                std::cerr << "Something went wrong when reading sorted file. Only "
                          << sorted_file.gcount ()
                          << " characters have been read in this page";
                return false;
            }
        }
    }
    else
    {
        std::cerr << "Could not open file for reading in <initialize_input_buffers" << std::endl;
        return false;
    }

    return true;
}


/**
 * Arguments:
 *   starting_pos_this_run - vector of offsets representing starting point for reading
 *     ending_pos_this_run - vector of offsets representing ending point for reading
 *           output_buffer - buffer to write data to
 *                 buffers - B - 1 buffers containing input data
 *    cur_sorted_file_name - name of file to read data from
 *      writable_file_name - name of file to write data to
 * 
 * Return:
 *   number of pages written in this merge
 */
size_t merge_this_run (PositionsType &starting_pos_this_run, const PositionsType &ending_pos_this_run,
                       BufferType &output_buffer, BuffersType &buffers, const std::string &cur_sorted_file_name,
                       const std::string &writable_file_name)
{
    size_t total_pages_merged = 0;

    std::ifstream sorted_file   (cur_sorted_file_name, std::ios::binary);
    std::ofstream writable_file (writable_file_name,   std::ios::binary | std::ios::app);

    if (sorted_file.is_open () && writable_file.is_open ())
    {
        while (starting_pos_this_run != ending_pos_this_run)
        {
            // If "output" buffer is not full yet, add more records.
            if (output_buffer.size () * RECORD_SIZE < pagesize)
            {
                // Find appropriate buffer
                size_t idx = find_correct_buffer_idx (buffers);
                BufferType &correct_buffer = buffers[idx];

                // Add record to "output" buffer
                output_buffer.push_back (correct_buffer.front ());

                // Remove it from the buffer it came from
                correct_buffer.erase (correct_buffer.begin ());

                // If that buffer is now empty, read more data into it (if necessary)
                if (correct_buffer.empty () && starting_pos_this_run[idx] != ending_pos_this_run[idx])
                {
                    correct_buffer = read_records_in_page (sorted_file, starting_pos_this_run[idx]);
                    if (correct_buffer.size () * RECORD_SIZE == pagesize)
                    {
                        // Read successfull, increment global counter and starting positions vector
                        totalpagesread++;
                        starting_pos_this_run[idx] += pagesize;
                    }
                    else
                    {
                        std::cerr << "Something went wrong when reading sorted file. Only "
                                  << sorted_file.gcount ()
                                  << " characters have been read in this page";
                        return total_pages_merged;
                    }
                }
            }
            else // output_buffer is full. Write its content to disk and clear it
            {
                if (write_records_in_page (writable_file, output_buffer))
                {
                    // Write successfull. Increment the counters and clear output buffer
                    total_pages_merged++;
                    totalpageswritten++;
                    output_buffer.clear ();
                }
                else
                {
                    std::cerr << "Something went wrong when writing into temporary file";
                    return total_pages_merged;
                }
            }
        }
    }
    else
    {
        std::cerr << "Could not open sorted or writable file. Passed in '"
                  << cur_sorted_file_name
                  << "', and '"
                  << writable_file_name
                  << "')."
                  << std::endl;
    }
    
    return total_pages_merged;
}


/**
 * Arguments:
 *            output_buffer - buffer to write data to
 *                  buffers - B - 1 buffers containing input data
 *       writable_file_name - name of file to write data to
 *   pages_written_on_merge - number of data pages written during this run
 * 
 * Return:
 *   pages leftover written to disk
 */
size_t write_leftover (BufferType &output_buffer, BuffersType &buffers,
                       const std::string &writable_file_name, size_t pages_written_on_merge)
{
    size_t pages_written_this_run = pages_written_on_merge;

    std::ofstream sorted_file (writable_file_name, std::ios::binary | std::ios::app);

    if (sorted_file.is_open ())
    {
        while (!output_buffer.empty () ||
               std::any_of (buffers.cbegin (), buffers.cend (),
                    [] (const auto &this_buf) { return !this_buf.empty (); }))
        {
            // Same strategy as in <merge_this_run>
            if (output_buffer.size () * RECORD_SIZE < pagesize)
            {
                size_t idx = find_correct_buffer_idx (buffers);
                BufferType &correct_buffer = buffers[idx];

                output_buffer.push_back (correct_buffer.front ());

                correct_buffer.erase (correct_buffer.begin ());
            }
            else
            {
                if (write_records_in_page (sorted_file, output_buffer))
                {
                    pages_written_this_run++;
                    totalpageswritten++;
                    output_buffer.clear ();
                }
                else
                {
                    std::cerr << "Something went wrong when writing into sorted file";
                    return pages_written_this_run;
                }
            }
        }
    }
    else
    {
        std::cerr << "Could not open file for writing in end of run phase" << std::endl;
    }

    return pages_written_this_run;
}


/**
 * Arguments:
 *   num_pages - number of data pages
 * 
 * Return:
 *   flag representing if everything went well
 */
bool Pass1_till_end (const unsigned long long num_pages)
{
    std::string cur_sorted_file_name = outputfilename, writable_file_name = TEMP_OUTPUT;
    
    // This is the number of sorted "blocks" of pages
    // For example, after Pass 0 every "sorted block" contains <num_buffers> pages,
    // except possibly the last "block", which may contain fewer pages
    auto num_sorted_blocks = static_cast<unsigned long long> (std::ceil (num_pages / numbuffers));

    // This offset delimits the boundaries of each "block"
    // For example, after Pass 0 every "block" is <num_buffers> * <pagesize> away from each other.
    // However, for each subsequent Pass, the block_offset grows exponentially
    auto block_offset = numbuffers * pagesize;

    do
    {
        // Algorithm:
        //
        // Within a pass and for every run, use two parallel vectors of positions.
        // The first vector contains the starting position of every "block" and 
        // the second vector contains the ending position of every "block".
        //
        // The current run ends when the vectors are equal.
        //
        // After every pass, use one file for reading and the other one for writing.
        // Delete the intermediate one at the end

        // Initialize the two vectors with appropriate offsets
        PositionsType start_pos_this_pass (num_sorted_blocks), end_pos_this_pass (num_sorted_blocks);
        for (unsigned long long i = 0; i < num_sorted_blocks; ++i)
        {
            start_pos_this_pass[i] += block_offset * i;
            end_pos_this_pass[i] += i == num_sorted_blocks - 1 ? num_pages * pagesize : block_offset * (i + 1);
        }

        // Delete file to write to. It will get created automatically
        remove (writable_file_name.c_str ());

        // Run ends when we processed every page
        size_t runs_so_far = 0;
        do
        {
            const auto start_pos_first_it = std::next (start_pos_this_pass.cbegin (),
                                                runs_so_far * (numbuffers - 1));
            const auto start_pos_last_it = std::next (start_pos_first_it,
                                                std::min (static_cast<std::ptrdiff_t> (numbuffers - 1),
                                                    start_pos_this_pass.cend () - start_pos_first_it));

            const auto end_pos_first_it = std::next (end_pos_this_pass.cbegin (),
                                             runs_so_far * (numbuffers - 1));
            const auto end_pos_last_it = std::next (end_pos_first_it,
                                             std::min (static_cast<std::ptrdiff_t> (numbuffers - 1),
                                                    end_pos_this_pass.cend () - end_pos_first_it));

            PositionsType starting_pos_this_run (start_pos_first_it, start_pos_last_it);
            const PositionsType ending_pos_this_run (end_pos_first_it, end_pos_last_it);

            // Initialize buffers that will contain data from disk.
            // B - 1 buffers to store data and a single one for output
            BuffersType buffers;
            BufferType output_buffer;

            if (!initialize_input_buffers (buffers, cur_sorted_file_name, starting_pos_this_run))
            {
                // Error message printed inside function calll
                return false;
            }

            // Merge using at most <numbuffers> - 1 buffers for input and 1 buffer for output
            size_t pages_merged = merge_this_run (starting_pos_this_run, ending_pos_this_run, output_buffer,
                                                  buffers, cur_sorted_file_name, writable_file_name);

            // Check that there is at least 1 full buffer in memory (exit condition of <merge_this_run>)
            if (0 == std::count_if (buffers.cbegin (), buffers.cend (),
                                      [] (const auto &buffer) { return buffer.size () * RECORD_SIZE == pagesize; }))
            {
                std::cerr << "Finished merging but there are no full buffers in memory" << std::endl;
                return false;
            }

            // Potentially, all <num_buffers> buffers are full after <merge_this_run> is called,
            // including the <output_buffer>
            size_t pages_written_this_run (write_leftover (output_buffer, buffers, writable_file_name, pages_merged));

            // Now we must have written all of them
            if (pages_written_this_run != (*std::prev (end_pos_last_it) - *start_pos_first_it) / pagesize)
            {
                std::cerr << "Finished writing but not all pages in run have been processed" << std::endl;
                return false;
            }

            runs_so_far++;
        } while (runs_so_far < std::ceil (num_sorted_blocks / (numbuffers - 1)));

        // Finished one pass
        // Update the number of sorted blocks (every run produced a sorted block), the offset,
        // increase Pass counter and swap file names
        num_sorted_blocks = static_cast<unsigned long long> (std::ceil (num_sorted_blocks / (numbuffers - 1)));
        block_offset *= numbuffers - 1;
        totalpasses++;
        cur_sorted_file_name.swap (writable_file_name);

        // File will be sorted when <block_offset> is as large as the file size
    } while (block_offset < num_pages * pagesize);

    // Before leaving this function make sure the sorted file is named correctly
    if (cur_sorted_file_name == TEMP_OUTPUT)
    {
        remove (outputfilename.c_str ());
        rename (TEMP_OUTPUT, outputfilename.c_str ());
    }

    remove (TEMP_OUTPUT);
    return true;
}


/**
 * Plan of attack for Question1:
 *  - Get user input and do some checking
 *  - Perform Pass 0. Use all B buckets. Read one page at the time, insert it in an ordered set
 *  - Perform as many passes as necessary after Pass 0. Use B - 1 buckets to read data from disk and one bucket to sort
 *    it in memory. For every pass, create two lists of indices, one that keeps track of starting positions of every
 *    run, one (constant) that keeps track of where each run is supposed to end. Repeat until the sorted block is as
 *    large as the original input file
 */
int main (int argc, char const *argv[])
{
    // Calculating run time in seconds, just out of curiosity
    auto start = std::chrono::steady_clock::now ();

    const std::string program_name (argv[0]);
    const std::string exe_name (program_name.substr (program_name.rfind ('/') + 1) + ".exe");

    if (argc -1 != NUM_REQUIRED_INPUTS)
    {
        std::cerr << "Usage: "
                  << exe_name
                  << " <.db input file name>"
                  << " <.db output sorted file name>"
                  << " <number of buffer pages in memory>"
                  << " <data page size, multiple of 64>"
                  << " <the index of the field to be sorted by; 0=First Name, 1=Last Name, 2=Email>"
                  << std::endl;
        return 1;
    }

    // Initialize global variables
    inputfilename  = argv[1];
    outputfilename = argv[2];
    numbuffers     = std::stof (argv[3]);
    pagesize       = std::stoul (argv[4]);
    field          = FieldType (std::stoi (argv[5]));

    // Some checking
    if (numbuffers < MIN_NUM_BUFFERS)
    {
        std::cerr << "Number of buffers must be at least " << MIN_NUM_BUFFERS << std::endl;
        return 1;
    }
    if (pagesize < MIN_PAGE_SIZE || pagesize % MIN_PAGE_SIZE != 0)
    {
        std::cerr << "Data page size must be a positive multiple of " << MIN_PAGE_SIZE << std::endl;
        return 1;
    }
    if (field != FieldType::FIRST_NAME && field != FieldType::LAST_NAME && field != FieldType::EMAIL)
    {
        std::cerr << "field can only be one of {0 - First Name, 1 - Last Name, 2 - Email}" << std::endl;
        return 1;
    }
    if (outputfilename == TEMP_OUTPUT)
    {
        std::cerr << "Please choose a different file name for the output file" << std::endl;
        return 1;
    }

    std::cout << "Running: " << exe_name << " " << argv[1]
              << " "         << argv[2]  << " " << argv[3]
              << " "         << argv[4]  << " " << argv[5] << std::endl;

    // Pass 0
    auto total_pages_and_pages_read = Pass0_full ();
    auto input_file_num_pages = total_pages_and_pages_read[0];
    auto pages_read = total_pages_and_pages_read[1];

    if (pages_read != input_file_num_pages) return 1;  // Some error occurred
    totalpasses++;

    // Pass 1 -> ...
    // Only necessary if <numbuffers> <input_file_num_pages>
    if (numbuffers < input_file_num_pages)
    {
        if (!Pass1_till_end (input_file_num_pages)) return 1;  // Some error occurred
    }

    // Get final number of pages in sorted file
    std::ifstream sorted_file (outputfilename, std::ios::binary | std::ios::ate);
    
    if (!sorted_file.is_open ()) return 1;  // Some error occurred

    const auto sorted_file_num_pages = sorted_file.tellg () / pagesize;

    auto end      = std::chrono::steady_clock::now ();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds> (end - start);
    auto seconds  = duration.count () / 1000.0F;

    std::cout << std::fixed << std::setprecision (2)
              << "Program finished running in "       << seconds << " seconds" << std::endl
              << "Number of buffer pages in memory: " << int (numbuffers)      << std::endl
              << "Number of pages in input file: "    << input_file_num_pages  << std::endl
              << "Number of pages in sorted file: "   << sorted_file_num_pages << std::endl
              << "Total number of pages read: "       << totalpagesread        << std::endl
              << "Total number of pages written: "    << totalpageswritten     << std::endl
              << "Total number of passes: "           << totalpasses           << std::endl;

    return 0;
}
