/*
 * Lino Lastella 1001237654 lastell1
 *
 * Program 2: read.hh
 *
 */


/*
 * This class takes care of keeping track of total time spent and actual reading of a page.
 * When a new Page is created, increment the static counter of child Page class by 1.
 * When a Page is about to exit the <read_page> method, increment the <total_time> counter accordingly
 */
class Page
{
protected:
  /* data */
  static double total_time;
	const unsigned int page_size;

	static unsigned int table_interior_counts;
	static unsigned int table_leaf_counts;
	static unsigned int index_interior_counts;
	static unsigned int index_leaf_counts;

public:
  Page (unsigned int page_size) : page_size (page_size) {}

	inline double get_total_time () const
	{
		return total_time;
	}
	inline void reset_total_time ()
	{
		total_time = 0;
	}

  static inline unsigned int get_table_interior_reads ()
	{
		return table_interior_counts;
	}
	static inline void reset_table_interior_reads ()
	{
		table_interior_counts = 0;
	}

	static inline unsigned int get_table_leaf_reads ()
	{
		return table_leaf_counts;
	}
	static inline void reset_table_leaf_reads ()
	{
		table_leaf_counts = 0;
	}

	static inline unsigned int get_index_interior_reads ()
	{
		return index_interior_counts;
	}
	static inline void reset_index_interior_reads ()
	{
		index_interior_counts = 0;
	}

	static inline unsigned int get_index_leaf_reads ()
	{
		return index_leaf_counts;
	}
	static inline void reset_index_leaf_reads ()
	{
		index_leaf_counts = 0;
	}

	void read_page ();
};


class TableInteriorPage : public Page
{
private:
	/* data */

public:
	TableInteriorPage (unsigned int page_size) : Page (page_size)
	{
		++table_interior_counts;
	}
};


class TableLeafPage : public Page
{
private:
	/* data */

public:
	TableLeafPage (unsigned int page_size) : Page (page_size)
	{
		++table_leaf_counts;
	}
};


class IndexInteriorPage : public Page
{
private:
	/* data */

public:
	IndexInteriorPage (unsigned int page_size) : Page (page_size)
	{
		++index_interior_counts;
	}
};


class IndexLeafPage : public Page
{
private:
	/* data */

public:
	IndexLeafPage (unsigned int page_size) : Page (page_size)
	{
		++index_leaf_counts;
	}
};
