"""
inferencer.py
====================================
Wrapper around the csvtype_ext Python bindings.

Examples
--------

::

    from csvtype.inferencer import Inferencer as Inf
    from csvtype.patterns import alpha_patterns, int_patterns, default_na_values
    import pandas as pd

    df = pd.DataFrame({
        'mostly_alpha': ['a', 'b', 'c', 1, 1.0],
        'mostly_int': [0, 'a', 2, 64, 'NA']
    })
    df.to_csv('test.csv', sep=';', index=False)

    col_type_patterns = {'int': int_patterns, 'alpha': alpha_patterns}
    inf = Inf(
        filepath='test.csv',
        delimiter=';',
        col_type_patterns=col_type_patterns,
        na_values=default_na_values
    )

    inf.infer_types()

    print(inf.col_names)
    ['mostly_alpha', 'mostly_int']

    print(inf.num_rows)
    5

    print(inf.col_type_candidates(pandas=True))
           mostly_alpha  mostly_int
    int             0.2         0.6
    alpha           0.6         0.2
    NA              0.0         0.2
    other           0.2         0.0

    print(inf.most_likely_col_types())
    {'mostly_alpha': 'alpha', 'mostly_int': 'int'}
"""

from csvtype_ext import Inf
from typing import Dict, List, Set
import logging
import os
import pandas as pd
from .patterns import default_patterns, default_na_values


class Inferencer(Inf):
    """
    A wrapper around the Python bindings of the underlying C++ code, the csvtype_ext Inf class.

    Attributes
    ----------
    filepath : str
        The path to the CSV file to be processed
    delimiter : str
        The delimiter of the CSV file to be processed
    col_type_patterns : dict of str, list of strings pairs, optional
        A dict of str, list of strings pairs. The string keys represent the names
        of column types, while the list of strings belonging to that key are regex
        patterns to be used for inferring that column type.
    na_values : set, optional
        A set of strings that represent literal NA values. No regex matching will be
        used for these, but rather direct string matching.
    multithreading : bool, optional
        Whether to use a new thread for each field in a row in the CSV file. This might
        severely slow down the inferencing, depending on many factors such as the number
        of columns and the number of CPU cores.
    save_types_file: bool, optional
        Whether to save the inferred types per field in a CSV file.
    types_filepath: str, optional
        The path to save the types file at.
    rolling_cache_window: int, optional
        A rolling cache is used that might speed up the inferencing process, which
        is true if it's likely that there are identical values in consecutive rows
        of one column. This rolling cached is kept for `rolling_cache_window` rows
        and then emptied. Changing this parameter might speed up or slow down the
        inferencing process.
    num_rows : int
        Read-only. The number of rows in the CSV file. Available after calling `infer_types()`.
    col_names : list of str
        Read-only. The column names of the CSV file. Available after calling `infer_types()`.
        Unnamed column will be named `Untitled_<index>`.
    """

    def __init__(
        self,
        filepath: str = None,
        delimiter: str = ',',
        col_type_patterns: Dict[str, List[str]] = default_patterns,
        na_values: Set[str] = default_na_values,
        multithreading: bool = False,
        save_types_file: bool = False,
        types_filepath: str = '',
        rolling_cache_window: int = 5
    ) -> Inf:
        """
        Initializes the inferencer instance.

        Parameters
        ----------
        filepath : str
            The path to the CSV file to be processed
        delimiter : str
            The delimiter of the CSV file to be processed
        col_type_patterns : dict of str, list of strings pairs, optional
            A dict of str, list of strings pairs. The string keys represent the names
            of column types, while the list of strings belonging to that key are PCRE-style
            regex patterns to be used for inferring that column type.
        na_values : set, optional
            A set of strings that represent literal NA values. No regex matching will be
            used for these, but rather direct string matching.
        multithreading : bool, optional
            Whether to use a new thread for each field in a row in the CSV file. This might
            severely slow down the inferencing, depending on many factors such as the number
            of columns and the number of CPU cores.
        save_types_file: bool, optional
            Whether to save the inferred types per field in a CSV file.
        types_filepath: str, optional
            The path to save the types file at.
        rolling_cache_window: int, optional
            A rolling cache is used that might speed up the inferencing process, which
            is true if it's likely that there are identical values in consecutive rows
            of one column. This rolling cached is kept for `rolling_cache_window` rows
            and then emptied. Changing this parameter might speed up or slow down the
            inferencing process.

        Raises
        ------
        OSError
            If the given `filepath` does not exist.

        Notes
        -----
        The `boost::regex` regex implementation is used in the `C++` source. It is currently
        not possible to pass any `boost::regex` flags from these Python bindings (such as
        `boost::regex::icase`) to the `C++` source. This might be added in a later release.

        By default, the `boost::regex::perl` flag is passed to the constructor, so you have
        to use PCRE regex patterns.

        Returns
        -------
        csvtype.Inferencer
            In instance of the Python wrapper of the csvtype Inferencer class.
        """
        if not os.path.exists(filepath):
            raise OSError('The filepath given does not exist. Please set a different filepath.')

        # Set default types output filepath if None is set
        if types_filepath is None and save_types_file:
            types_filepath = f'{filepath}.ctypes'
            if os.path.exists(types_filepath):
                logging.warning('''The filepath for the types file already exists. It will be overwritten unless you
                    set a different path.''')

        super().__init__(
            filepath=filepath,
            delimiter=delimiter,
            col_type_patterns=col_type_patterns,
            na_values=na_values,
            multithreading=multithreading,
            save_types_file=save_types_file,
            types_filepath=types_filepath,
            rolling_cache_window=rolling_cache_window
        )

    def infer_types(self) -> None:
        """
        Infer the most likely column types based on the col type regex patterns
        given to initializer.
        """
        super().infer_types()

    def col_type_candidates(self, pandas: bool = False) -> [Dict, pd.DataFrame]:
        """
        Computes col type candidate ratios per column based on col type counts
        per column as given by the C++ inferencer.

        Parameters
        ----------
        pandas : bool
            Whether the output should be given as a Pandas df.

        Returns
        -------
        dict, pandas.DataFrame
            A datastructure that gives a ratio (between 0 and 1) for each column and column type
            combination indicating the likeliness the column is of the specific column type.
        """
        col_type_candidates = self.get_col_type_candidates()
        num_rows = self.num_rows
        pretty_candidates = {}

        for col, candidates in col_type_candidates.items():
            pretty_candidates[col] = {}
            for col_type, count in candidates.items():
                pretty_candidates[col][col_type] = count / num_rows

        # Convert nested dict to Pandas
        if pandas:
            index = self.col_types
            data = {}
            for col in self.col_names:
                data[col] = []
                for col_type in index:
                    data[col].append(pretty_candidates[col][col_type])
            return pd.DataFrame(index=index, data=data)

        return pretty_candidates

    def most_likely_col_types(self) -> Dict:
        """
        Computes most likely col type for each column.

        Returns
        -------
        dict
            A dict with columns as keys and the most likely col types as values.
        """
        candidates = self.col_type_candidates(pandas=True)
        return candidates.idxmax().to_dict()

    @property
    def filepath(self):
        return self.get_filepath()

    @filepath.setter
    def filepath(self, val):
        return self.set_filepath(val)

    @property
    def delimiter(self):
        return self.get_delimiter()

    @delimiter.setter
    def delimiter(self, val):
        return self.set_delimiter(val)

    @property
    def na_values(self):
        return self.get_na_values()

    @na_values.setter
    def na_values(self, val):
        return self.set_na_values(val)

    @property
    def col_type_patterns(self):
        return self.get_col_type_patterns()

    @col_type_patterns.setter
    def na_values(self, val):
        return self.set_col_type_patterns(val)

    @property
    def multithreading(self):
        return self.get_multithreading()

    @multithreading.setter
    def multithreading(self, val):
        return self.set_multithreading(val)

    @property
    def save_types_file(self):
        return self.get_save_types_file()

    @save_types_file.setter
    def save_types_file(self, val):
        return self.set_save_types_file(val)

    @property
    def types_filepath(self):
        return self.get_types_filepath()

    @types_filepath.setter
    def types_filepath(self, val):
        return self.set_types_filepath(val)

    @property
    def col_types(self):
        return list(self.col_type_patterns.keys()) + ['NA', 'other']
