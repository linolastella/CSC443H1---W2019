/*
 * Lino Lastella 1001237654 lastell1
 *
 * Program 1: write.cc
 *
 * Create 4 SQLite databases by loading the CSV file as a table named "Employee".
 *
 * Compiled with GCC 7.3.0
 */


#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <map>
#include <regex>
#include <string>
#include "sqlite3.h"


#define CSV_FILE "500000 Records.csv"
#define NUM_ROWS 386791 /* I removed duplicates from the csv file */
#define NUM_COLS 37


/*
 * Arguments:
 *
 *   all_records - 2d vector to be filled-in with entries from the csv file
 */
void load_csv (std::vector<std::vector<std::string>> &all_records)
{
  std::ifstream csv_file (CSV_FILE);

  if (csv_file.is_open ())
  {
    std::string cur_row, cur_cel;

    while (getline (csv_file, cur_row))
    {
      all_records.emplace_back ();
      auto &row = all_records.back ();

      std::stringstream cell_stream (cur_row);

      while (getline (cell_stream, cur_cel, ','))
      {
        row.push_back (cur_cel);
      }
    }

    csv_file.close ();
  }
  else
  {
    std::cerr << "Could not read input file." << std::endl;
  }
}


/*
 * Arguments:
 *
 *      all_records - 2d vector containing the csv file entries
 *   col_to_max_len - map from column name to max length for that column,
 *                    to be filled-in
 */
void calculate_max_len (const std::vector<std::vector<std::string>> &all_records,
                        std::map<std::string, unsigned short> &col_to_max_len)
{
  const auto &header = all_records[0];

  for (unsigned short col_num = 1; col_num < NUM_COLS; ++col_num)
  {
    const auto &col_name = header[col_num];

    auto max_col = std::max_element (std::next (all_records.cbegin ()), all_records.cend (),
                                    [col_num] (const auto &lhs, const auto &rhs) -> bool
      {
          return lhs[col_num].length () < rhs[col_num].length ();
      });

    col_to_max_len[col_name] = (*max_col)[col_num].length ();
  }
}


/*
 * Arguments:
 *
 *      all_records - 2d vector containing the csv file entries
 *   col_to_max_len - map from column name to max length for that column
 */
std::string get_sql_create_table (const std::vector<std::vector<std::string>> &all_records,
                                  std::map<std::string, unsigned short> &col_to_max_len)
{
  const auto &header = all_records[0];
  std::string SQL = "DROP TABLE IF EXISTS Employee;"
                    "CREATE TABLE Employee(EMP_ID INT";

  for (auto col_name = std::next (header.cbegin ()); col_name != header.cend (); ++col_name)
  {
    std::string field_name ("\"" + *col_name + "\"");

    SQL += ",  "                                      +
           field_name                                 +
           " CHAR("                                   +
           std::to_string (col_to_max_len[*col_name]) +
           ")";
  }

  SQL += ");";
  return SQL;
}


/*
 * Arguments:
 *
 *           entry - csv entry to pad and escape quotes from
 *        col_name - name of the current column
 *  col_to_max_len - map from column name to max length for that column
 */
std::string pad_and_replace_quote (const std::string &entry, const std::string &col_name,
                                   std::map<std::string, unsigned short> &col_to_max_len)
{  
  // pad with as many whitespaces as needed
  auto new_entry = entry + std::string (col_to_max_len[col_name] - entry.length (), ' ');

  // replace single quote with backtick
  std::replace (new_entry.begin (), new_entry.end (), '\'', '`');

  return new_entry;
}


/*
 * Arguments:
 *
 *     all_records - 2d vector containing the csv file entries
 *  col_to_max_len - map from column name to max length for that column
 */
std::string get_sql_insert (const std::vector<std::vector<std::string>> &all_records,
                            std::map<std::string, unsigned short> &col_to_max_len)
{
  std::string all_inserts = "INSERT OR IGNORE INTO Employee VALUES (";

  for (auto record = std::next (all_records.cbegin ()); record != all_records.cend (); ++record)
  {
    all_inserts += record->at (0);   // EMP_ID is the only INT, so no need for ' '

    for (unsigned short col_num = 1; col_num < NUM_COLS; ++col_num)
    {
      auto this_entry = pad_and_replace_quote (record->at (col_num), all_records[0][col_num], col_to_max_len);

      all_inserts += ", '" + this_entry + "'";
    }

    all_inserts += "), (";
  }

  return all_inserts.replace (all_inserts.end () - 3, all_inserts.end (), ";");
}


/*
 * Arguments:
 *
 *   err_msg - error message from sqlite3_exec (...) to be printed to std::cerr
 */
void print_err_msg (char *err_msg)
{
  std::cerr << "sqlite3_exec () was not successfull. Error message: "
            << std::string (err_msg)
            << std::endl;
}


/*
 * Arguments:
 *
 *  sql_create_table - "CREATE TABLE" statement to be modified for part d)
 */
std::string modify_create_statement (const std::string &sql_create_table)
{
  // make EMP_ID primary key
  std::string mod_create (std::regex_replace (sql_create_table,
                                              std::regex (" INT, "),
                                              " INT PRIMARY KEY, "));

  // remove semicolon at the end of the statement
  mod_create.replace (mod_create.end () - 1, mod_create.end (), "");

  // allow index to be clustered
  mod_create += " WITHOUT ROWID;";

  return mod_create;
}


/*
 * Arguments:
 *
 *                db  - pointer to database object to open
 *  default_page_size - if true leave defautl page size, otherwise set to 16 KB
 *                      default page size is 4KB starting from SQLite version 3.12.0 (2016-03-29)
 *       index_on_ID  - if true create a primary index on EMP_ID
 *   clustered_index  - only true if index_on_ID also true, in which case the
 *                      index created previously should be clustered
 *  sql_create_table  - SQL statement to create the 'Employee' table
 *  sql_insert_values - SQL statement to insert every record
 *           db_name  - name of the database file
 */
int create_db (sqlite3 *db, bool default_page_size, bool index_on_ID, bool clustered_index,
               const std::string &sql_create_table, const std::string &sql_insert_values,
               const char *db_name)
{
  int rc = sqlite3_open (db_name, &db);

  if (rc == SQLITE_OK)
  {
    // database opened successfully
    char *error_msg = nullptr;

    if (!default_page_size)
    {
      rc = sqlite3_exec (db, "PRAGMA page_size = 16384", nullptr, nullptr, &error_msg);

      if (rc != SQLITE_OK)
      {
        print_err_msg (error_msg);
        sqlite3_free (error_msg);
        sqlite3_close (db);
        return rc;
      }
    }

    // create table
    if (index_on_ID && clustered_index)
    {
      // Add "WITHOUT ROWID" and "PRIMARY KEY" to the CREATE TABLE statement
      rc = sqlite3_exec (db, (modify_create_statement (sql_create_table)).c_str (), nullptr, nullptr, &error_msg);
    }
    else
    {
      rc = sqlite3_exec (db, sql_create_table.c_str (), nullptr, nullptr, &error_msg);
    }

    if (rc == SQLITE_OK)
    {
      // table created successfully
      if (index_on_ID && !clustered_index) // PRIMARY KEY in table definition automatically creates an index,
                                           // so only create one on third table since there is P.K.
      {
        auto sql_create_index = "CREATE UNIQUE INDEX Emp_id ON Employee(EMP_ID);";

        rc = sqlite3_exec (db, sql_create_index, nullptr, nullptr, &error_msg);
        if (rc != SQLITE_OK)
        {
          print_err_msg (error_msg);
          sqlite3_free (error_msg);
          sqlite3_close (db);
          return rc;
        }
      }

      // insert values
      rc = sqlite3_exec (db, sql_insert_values.c_str (), nullptr, nullptr, &error_msg);
      if (rc == SQLITE_OK)
      {
        // values inserted successfully
        std::cout << "Values inserted in " << db_name << std::endl;
      }
      else
      {
        print_err_msg (error_msg);
        sqlite3_free (error_msg);
      }
    }
    else
    {
      print_err_msg (error_msg);
      sqlite3_free (error_msg);
    }
  }
  else
  {
    std::cerr << "Could not open database " << db_name << std::endl;
  }

  sqlite3_close (db);
  return rc;
}


int main (int argc, const char *argv[])
{
  // read csv file
  std::vector<std::vector<std::string>> all_records;
  all_records.reserve (NUM_ROWS);
  load_csv (all_records);

  // calculate max length of each column but Emp ID
  std::map<std::string, unsigned short> col_to_max_len;
  calculate_max_len (all_records, col_to_max_len);

  // SQL statements needed
  std::string sql_create_table (get_sql_create_table (all_records, col_to_max_len));
  std::string sql_insert_values (get_sql_insert (all_records, col_to_max_len));

  // create sqlite databases
  sqlite3 *db_a, *db_b, *db_c, *db_d;
  int rc_a, rc_b, rc_c, rc_d;

  rc_a = create_db (db_a, /*default_page_size=*/true, /*index_on_ID=*/false, /*clustered_index=*/false,
                    sql_create_table, sql_insert_values, "database_a.db");
  if (rc_a != SQLITE_OK)
  {
    std::cerr << "Something went wrong..." << std::endl;
    return 1;
  }

  rc_b = create_db (db_b, /*default_page_size=*/false, /*index_on_ID=*/false, /*clustered_index=*/false,
                    sql_create_table, sql_insert_values, "database_b.db");
  if (rc_b != SQLITE_OK)
  {
    std::cerr << "Something went wrong..." << std::endl;
    return 1;
  }

  rc_c = create_db (db_c, /*default_page_size=*/true, /*index_on_ID=*/true, /*clustered_index=*/false,
                    sql_create_table, sql_insert_values, "database_c.db");
  if (rc_c != SQLITE_OK)
  {
    std::cerr << "Something went wrong..." << std::endl;
    return 1;
  }

  rc_d = create_db (db_d, /*default_page_size=*/true, /*index_on_ID=*/true, /*clustered_index=*/true,
                    sql_create_table, sql_insert_values, "database_d.db");
  if (rc_d != SQLITE_OK)
  {
    std::cerr << "Something went wrong..." << std::endl;
    return 1;
  }

  std::cout << "finished running" << std::endl;
  return 0;
}
