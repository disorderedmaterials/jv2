# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
import json
import pandas as pd
import pytest

from jv2backend.instrument import Instrument
from jv2backend.journal import Journal

INSTRUMENT_NAME = "FAKE"


@pytest.fixture()
def journal_fake(sample_journal_dataframe):
    return _create_fake_journal(sample_journal_dataframe)


def _create_fake_journal(sample_dataframe):
    return Journal(Instrument(INSTRUMENT_NAME), sample_dataframe)


def test_default_journal_creation_stores_instrument_and_data(sample_journal_dataframe):
    journal = _create_fake_journal(sample_journal_dataframe)

    assert journal.instrument.name == INSTRUMENT_NAME
    assert journal.run_count == len(sample_journal_dataframe)


def test_to_json_gives_dataframe_with_same_information(sample_journal_dataframe):
    journal = _create_fake_journal(sample_journal_dataframe)

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
def test_search_by_user_name_field_uses_a_contains_check_not_exact_match(
    case_sensitive, journal_fake
):
    run_field, user_input = "user_name", "username"
    search_results = journal_fake.search(run_field, user_input, case_sensitive)

    if case_sensitive:
        assert search_results.run_count == 0
    else:
        assert search_results.run_count == 3


@pytest.mark.parametrize(
    "case_sensitive",
    [True, False],
)
def test_search_by_experiment_identifier_uses_exact_string_matching(
    case_sensitive, journal_fake
):
    run_field, user_input = "experiment_identifier", "123456"

    search_results = journal_fake.search(run_field, user_input, case_sensitive)
    assert search_results.run_count == 0

    user_input = "1234567"
    search_results = journal_fake.search(run_field, user_input, case_sensitive)
    assert search_results.run_count == 2


@pytest.mark.parametrize(
    "case_sensitive",
    [True, False],
)
def test_search_by_title_uses_a_contains_check_and_not_exact_match(
    case_sensitive, journal_fake
):
    run_field, user_input = "title", "My cycle"

    search_results = journal_fake.search(run_field, user_input, case_sensitive)

    if case_sensitive:
        assert search_results.run_count == 0
    else:
        assert search_results.run_count == 1
