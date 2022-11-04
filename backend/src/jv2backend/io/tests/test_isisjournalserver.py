# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
import pytest
import requests
from unittest.mock import MagicMock

from jv2backend.io.isis.isisjournalserver import ISISJournalServer

FAKE_SERVER_ADDRESS = "http://fake.url/testing"
FAKE_INSTRUMENT_NAME = "FAKE"


def test_journal_filenames_parsed_as_expected_on_successful_response(requests_mock, sample_journallist_xml):
    requests_mock.get(_fake_instrument_journallist_url(),
        content=sample_journallist_xml
    )
    server = ISISJournalServer(FAKE_SERVER_ADDRESS)

    journal_filenames = server.journal_filenames(FAKE_INSTRUMENT_NAME)

    assert journal_filenames is not None
    assert len(journal_filenames) == 1

def test_journal_filenames_returns_None_on_http_error(requests_mock):
    requests_mock.get(_fake_instrument_journallist_url(),
        content=b'Not Found', status_code=404,
    )
    server = ISISJournalServer(FAKE_SERVER_ADDRESS)

    assert server.journal_filenames(FAKE_INSTRUMENT_NAME) is None

# private

def _fake_instrument_journallist_url():
    return FAKE_SERVER_ADDRESS + f"/ndx{FAKE_INSTRUMENT_NAME.lower()}/journal_main.xml"
