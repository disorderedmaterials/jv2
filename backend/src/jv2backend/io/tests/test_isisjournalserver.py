# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from jv2backend.journal import Journal
from jv2backend.journalfilelist import JournalFileList
from jv2backend.io.isis.isisjournalserver import ISISJournalServer

import pytest

FAKE_SERVER_ADDRESS = "http://fake.url/testing"
FAKE_INSTRUMENT_NAME = "FAKE"


def test_journal_filenames_parsed_as_expected_on_successful_response(
    requests_mock, sample_journallist_xml
):
    requests_mock.get(
        _fake_instrument_journallist_url(FAKE_INSTRUMENT_NAME),
        content=sample_journallist_xml,
    )
    server = ISISJournalServer(FAKE_SERVER_ADDRESS)

    journal_filenames = server.journal_filenames(FAKE_INSTRUMENT_NAME)

    assert isinstance(journal_filenames, JournalFileList)
    assert len(journal_filenames) == 1


def test_journal_filenames_raises_Exception_on_http_error(requests_mock):
    requests_mock.get(
        _fake_instrument_journallist_url(FAKE_INSTRUMENT_NAME),
        content=b"Not Found",
        status_code=404,
    )
    server = ISISJournalServer(FAKE_SERVER_ADDRESS)

    with pytest.raises(Exception, match=".*404.*"):
        server.journal_filenames(FAKE_INSTRUMENT_NAME)


def test_journal_returned_on_successful_response(requests_mock, sample_journal_xml):
    instrument_name, journal_filename = "ALF", "journal_21_1.xml"
    requests_mock.get(
        _fake_instrument_journal_url(instrument_name, journal_filename),
        content=sample_journal_xml,
    )
    server = ISISJournalServer(FAKE_SERVER_ADDRESS)

    journal = server.journal(instrument_name, journal_filename)

    assert isinstance(journal, Journal)
    assert journal.run_count() == 3


def test_journal_call_raises_Exception_on_http_error(requests_mock):
    instrument_name, journal_filename = "ALF", "bad.xml"
    requests_mock.get(
        _fake_instrument_journal_url(instrument_name, journal_filename),
        content=b"Not Found",
        status_code=404,
    )
    server = ISISJournalServer(FAKE_SERVER_ADDRESS)

    with pytest.raises(Exception, match=".*404.*"):
        server.journal(instrument_name, journal_filename)


# private
def _fake_instrument_journallist_url(instrument_name: str) -> str:
    return FAKE_SERVER_ADDRESS + f"/ndx{instrument_name.lower()}/journal_main.xml"


def _fake_instrument_journal_url(instrument_name: str, journal_filename: str) -> str:
    return FAKE_SERVER_ADDRESS + f"/ndx{instrument_name.lower()}/{journal_filename}"
