# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
import json
import pandas as pd
import pytest

from jv2backend.instrument import Instrument
from jv2backend.journal import Journal

INSTRUMENT_NAME = "FAKE"


def test_default_journal_creation_stores_instrument_and_data(sample_journal_dataframe):
    journal = _create_test_journal(sample_journal_dataframe)

    assert journal.instrument.name == INSTRUMENT_NAME
    assert journal.run_count == len(sample_journal_dataframe)


def test_to_json_gives_dataframe_with_same_information(sample_journal_dataframe):
    journal = _create_test_journal(sample_journal_dataframe)

    runs = journal.to_json()

    records = json.loads(runs)
    sample_records = json.loads(sample_journal_dataframe.to_json(orient="records"))
    assert len(records) == len(sample_journal_dataframe)
    for response_record, sample_record in zip(records, sample_records):
        diff = set(response_record.items()) ^ set(sample_record.items())
        assert len(diff) == 0


@pytest.mark.parametrize(
    "case_sensitive",
    [True, False],
)
def test_search_by_user_name_field(case_sensitive, sample_journal_dataframe):
    journal = _create_test_journal(sample_journal_dataframe)
    run_field, user_input = "user_name", "username"

    search_results = journal.search(run_field, user_input, case_sensitive)

    if case_sensitive:
        assert search_results.run_count == 0
    else:
        assert search_results.run_count == 3


# def test_extend_adds_the_runs_from_the_given_journal_with_caller(
#     sample_journal_dataframe,
# ):
#     journal_a, journal_b = _create_test_journal(
#         sample_journal_dataframe
#     ), _create_test_journal(sample_journal_dataframe)
#     journal_a_run_count_orig = journal_a.run_count

#     journal_a.extend(journal_b)

#     assert journal_a.run_count == journal_a_run_count_orig + journal_b.run_count


# Private helpers
def _create_test_journal(sample_data: pd.DataFrame) -> Journal:
    return Journal(Instrument("FAKE"), sample_data)
