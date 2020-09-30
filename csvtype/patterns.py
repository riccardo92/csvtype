"""
patterns.py
====================================
Regex patterns for common column types

Attributes
----------
default_patterns : dict of str, list of str pairs
    Default `col_type_patterns` used by `Inferencer.__init__` in case
    no col type patterns dict were given. Includes patterns for float, int,
    bool, gender and date.

default_na_values : set of str
    Default `na_values` used by `Inferencer.__init__` in case
    no NA values set was given. Includes most common NA values,
    including those used in MS Excel.
"""

alpha_patterns = ['^[a-zA-Z]+$']
float_patterns = ['^([-+]?\\d*\\.\\d+)$']
int_patterns = ['^[-+]?\\d+$']
bool_patterns = ['^(true|false|yes|no|ja|nee|y|n|j|0|1|t|f|waar|onwaar)$']
date_patterns = [
    '^(\\d{1,2})(-|\\.|/)(\\d{1,2})(-|\\.|/)(\\d{2}|\\d{4})(\\s\\d{1,2}:\\d{1,2}:\\d{1,2})?(\\d{1,2}:\\d{1,2})?$',
    '^(\\d{1,2})/(\\d{1,2})/(\\d{2}|\\d{4})(\\s\\d{1,2}:\\d{1,2}:\\d{1,2})?(\\d{1,2}:\\d{1,2})?$',
    '^(\\d{2}|\\d{4})(-|\\.|/)(\\d{1,2})(-|\\.|/)(\\d{1,2})(\\s\\d{1,2}:\\d{1,2}:\\d{1,2})?(\\d{1,2}:\\d{1,2})?'
]

default_patterns = {
    'alpha': alpha_patterns,
    'float': float_patterns,
    'int': int_patterns,
    'bool': bool_patterns,
    'date': date_patterns
}

default_na_values = {
    '',
    '#N/A',
    '#N/A',
    'N/A',
    '#NA',
    'NA',
    '-1.#IND',
    '-1.#QNAN',
    '-NaN',
    '-nan',
    '1.#IND',
    '1.#QNAN',
    'N/A',
    'NULL',
    'NaN',
    'n/a',
    'nan',
    'null'
}
