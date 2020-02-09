/*
 * Lino Lastella 1001237654 lastell1
 *
 * Program 2: read.cc
 *
 * Compiled with GCC 7.3.0
 */


#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <chrono>
#include "read.hh"


/**************************************************************************************************
 * Page class and children classes definitions                                                    *
 **************************************************************************************************/

double Page::total_time = 0;

unsigned int Page::table_interior_counts = 0;
unsigned int Page::table_leaf_counts = 0;
unsigned int Page::index_interior_counts = 0;
unsigned int Page::index_leaf_counts = 0;

void Page::read_page ()
{
  auto start = std::chrono::system_clock::now ();
  
  /* read page code here */

  auto end = std::chrono::system_clock::now ();
  std::chrono::duration<double> elapsed_seconds = end - start;

  total_time += elapsed_seconds.count ();
}

/**************************************************************************************************
 * end of definitions                                                                             *
 **************************************************************************************************/


/*
 * Arguments:
 *
 *  file_stream - input file stream to read from
 */
unsigned int get_db_page_size (std::ifstream &file_stream)
{
  // page size is 2 bytes offset of 16 from bof
  constexpr auto OFFSET = 16;
  constexpr auto SIZE = 2;

  std::vector<char> page_size_buf (SIZE);

  file_stream.seekg (OFFSET, std::ios::beg);
  file_stream.read (&page_size_buf[0], SIZE);

  unsigned int page_size = (page_size_buf[0] << 8) | page_size_buf[1];

  return page_size;
}


int main (int argc, char const *argv[])
{
  const std::vector<const char *> db_files
  {
    "database_a.db", "database_b.db", "database_c.db", "database_d.db"
  };

  for (auto cur_db_file : db_files)
  {
    std::ifstream file_stream (cur_db_file, std::ifstream::binary | std::ios::ate);

    if (file_stream.is_open ())
    {
      auto file_size = file_stream.tellg ();
      auto page_size = get_db_page_size (file_stream);
      auto num_pages = file_size / page_size;

      // read every page into memory
      for (int i = 0; i < num_pages; ++i)
      {
        std::vector<char> cur_page_chars (page_size);
        file_stream.seekg (page_size * i, std::ios::beg);
        file_stream.read (&cur_page_chars[0], page_size);
      }

      file_stream.close ();
    }
    else
    {
      std::cerr << "Something went wrong... Could not open db file " << cur_db_file << std::endl;
    }
  }
  
  return 0;
}
