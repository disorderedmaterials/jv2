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
    assert journal.run_count() == len(sample_journal_dataframe)


def test_runs_gives_dataframe_with_same_information(sample_journal_dataframe):
    journal = _create_test_journal(sample_journal_dataframe)

    runs = journal.runs()

    records = json.loads(runs)
    assert len(records) == len(sample_journal_dataframe)
    for response_record, sample_record in zip(
        records, sample_journal_dataframe.to_dict(orient="records")
    ):
        assert response_record == sample_record


def test_runs_raies_error_on_unsupported_format(sample_journal_dataframe):
    journal = _create_test_journal(sample_journal_dataframe)

    with pytest.raises(ValueError):
        journal.runs(format="text")


# Private helpers
def _create_test_journal(sample_data: pd.DataFrame) -> Journal:
    return Journal(Instrument("FAKE"), sample_data)
