/**
 * Lino Lastella 1001237654 lastell1
 *
 * Constants shared across the assignment
 */


#pragma once


constexpr unsigned short FIRST_NAME_MAX_LEN  = 12;
constexpr unsigned short LAST_NAME_MAX_LEN   = 14;
constexpr unsigned short EMAIL_MAX_LEN       = 38;
constexpr unsigned short RECORD_SIZE         = FIRST_NAME_MAX_LEN + LAST_NAME_MAX_LEN + EMAIL_MAX_LEN;
constexpr unsigned short MIN_PAGE_SIZE       = 64;

enum class FieldType : unsigned short
{
    FIRST_NAME,
    LAST_NAME,
    EMAIL
};

extern FieldType field;
