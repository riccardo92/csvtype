from csvtype.inferencer import Inferencer
import pytest


@pytest.mark.parametrize(
    ('filepath, delimiter, col_type_patterns, na_values, multithreading, save_types_file, '
     'types_filepath, expected_candidates'),
    [
        (
            'tests/single_column.csv',
            ',',
            {'int': ['^[-+]?\\d+$'], 'alpha': ['^[a-zA-Z]+$']},
            {'NA'},
            False,
            False,
            '',
            {'col_1': {'int': 0.25, 'alpha': 0.5, 'NA': 0.25, 'other': 0.0}}
        ),
        (
            'tests/multi_column.csv',
            ';',
            {'int': ['^[-+]?\\d+$'], 'alpha': ['^[a-zA-Z]+$']},
            {'NA'},
            True,
            False,
            '',
            {
                'col_1': {'int': 0.75, 'alpha': 0.25, 'NA': 0.0, 'other': 0.0},
                'col_2': {'int': 0.0, 'alpha': 3/8, 'NA': 3/8, 'other': 0.25}
            }
        )
    ]
)
def test_inferencer(
    filepath,
    delimiter,
    col_type_patterns,
    na_values,
    multithreading,
    save_types_file,
    types_filepath,
    expected_candidates
):
    """
    Tests if the Inferencer correctly infers types.
    """
    # Init inferencer using default col type patterns
    inf = Inferencer(
        filepath=filepath,
        delimiter=delimiter,
        col_type_patterns=col_type_patterns,
        na_values=na_values,
        multithreading=multithreading,
        save_types_file=save_types_file,
        types_filepath=types_filepath
    )

    # Infer column types
    inf.infer_types()

    # Get candidates
    col_type_candidates = inf.col_type_candidates()

    assert col_type_candidates == expected_candidates


@pytest.mark.parametrize(
    'filepath, expected_exception',
    [('non_existing.csv', OSError)]
)
def test_non_existing_path_error(filepath, expected_exception):
    """
    Tests if the Inferencer throws an Exception if the filepath
    does not exist.
    """
    with pytest.raises(expected_exception):
        _ = Inferencer(
            filepath=filepath,
            delimiter=None,
            col_type_patterns=None,
            na_values=None,
            multithreading=False,
            save_types_file=False,
            types_filepath=None
        )
