# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from jv2backend.journal import Journal
from jv2backend.journalFileList import JournalFileList
from jv2backend.io.isis.isisJournalServer import ISISJournalServer

import pytest

FAKE_SERVER_ADDRESS = "http://fake.url/testing"
FAKE_INSTRUMENT_NAME = "FAKE"


@pytest.fixture()
def server_faker(requests_mock, sample_journallist_xml, sample_journal_xml):
    def _fixture(instrument_name):
        server = ISISJournalServer(FAKE_SERVER_ADDRESS)
        requests_mock.get(
            _fake_instrument_journallist_url(instrument_name),
            content=sample_journallist_xml,
            headers={"Last-Modified": "Fri, 04 Nov 2022 10:34:44 GMT"},
        )
        for journal_filename in server.journal_filenames(instrument_name):
            requests_mock.get(
                _fake_instrument_journal_url(instrument_name, journal_filename),
                content=sample_journal_xml(journal_filename),
            )

        return server

    return _fixture


def test_cyclename_returns_expected_filename_when_cycle_word_missing():
    assert ISISJournalServer.filename("21_1") == "journal_21_1.xml"

def test_cyclename_returns_expected_filename_when_cycle_word_included():
    assert ISISJournalServer.filename("cycle_21_1") == "journal_21_1.xml"


def test_journal_filenames_parsed_as_expected_on_successful_response(server_faker):
    server = server_faker(FAKE_INSTRUMENT_NAME)

    journal_filenames = server.journal_filenames(FAKE_INSTRUMENT_NAME)

    assert isinstance(journal_filenames, JournalFileList)
    assert len(journal_filenames) == 2


def test_journal_filenames_raises_Exception_on_http_error(requests_mock):
    requests_mock.get(
        _fake_instrument_journallist_url(FAKE_INSTRUMENT_NAME),
        content=b"Not Found",
        status_code=404,
    )
    server = ISISJournalServer(FAKE_SERVER_ADDRESS)

    with pytest.raises(Exception, match=".*404.*"):
        server.journal_filenames(FAKE_INSTRUMENT_NAME)


def test_journal_raises_ValueError_if_filename_cyclename_not_provided():
    server = ISISJournalServer(FAKE_SERVER_ADDRESS)
    with pytest.raises(ValueError):
        server.journal("ALF")


@pytest.mark.parametrize(
    "call_args", [{"filename": "journal_21_1.xml"}, {"cyclename": "21_1"}]
)
def test_journal_returned_on_successful_response(
    call_args, requests_mock, sample_journal_xml
):
    instrument_name = "ALF"
    requests_mock.get(
        _fake_instrument_journal_url(instrument_name, "journal_21_1.xml"),
        content=sample_journal_xml(),
    )
    server = ISISJournalServer(FAKE_SERVER_ADDRESS)

    journal = server.journal(instrument_name, **call_args)

    assert isinstance(journal, Journal)
    assert journal.run_count == 3


def test_journal_call_raises_Exception_on_http_error(requests_mock):
    instrument_name, journal_filename = "ALF", "bad.xml"
    requests_mock.get(
        _fake_instrument_journal_url(instrument_name, journal_filename),
        content=b"Not Found",
        status_code=404,
    )
    server = ISISJournalServer(FAKE_SERVER_ADDRESS)

    with pytest.raises(Exception, match=".*404.*"):
        server.journal(instrument_name, filename=journal_filename)


def test_check_for_journal_filenames_update_returns_latest_filename_when_changed_since_last_request(
    requests_mock, sample_journallist_xml
):
    instrument_name = "ALF"
    server = ISISJournalServer(FAKE_SERVER_ADDRESS)

    requests_mock.get(
        _fake_instrument_journallist_url(instrument_name),
        content=sample_journallist_xml,
        headers={"Last-Modified": "Fri, 04 Nov 2022 10:34:44 GMT"},
    )
    server.journal_filenames(instrument_name)
    requests_mock.head(
        _fake_instrument_journallist_url(instrument_name),
        headers={"Last-Modified": "Fri, 11 Nov 2022 10:34:44 GMT"},
    )

    result = server.check_for_journal_filenames_update(instrument_name)

    assert result == "journal_20_2.xml"


def test_check_for_journal_filenames_update_returns_Nonewhen_no_change_since_last_request(
    requests_mock, sample_journallist_xml
):
    instrument_name = "ALF"
    server = ISISJournalServer(FAKE_SERVER_ADDRESS)

    requests_mock.get(
        _fake_instrument_journallist_url(instrument_name),
        content=sample_journallist_xml,
        headers={"Last-Modified": "Fri, 04 Nov 2022 10:34:44 GMT"},
    )
    server.journal_filenames(instrument_name)
    requests_mock.head(
        _fake_instrument_journallist_url(instrument_name),
        headers={"Last-Modified": "Fri, 04 Nov 2022 10:34:44 GMT"},
    )

    result = server.check_for_journal_filenames_update(instrument_name)

    assert result is None


def test_search_by_user_name_search_across_all_journals(server_faker):
    instrument_name = "ALF"
    server = server_faker(instrument_name)
    run_field, user_input = "user_name", "Username2"
    search_results = server.search(instrument_name, run_field, user_input)

    assert search_results.run_count == 3


def test_search_by_experiment_identifier_search_across_all_journals(server_faker):
    instrument_name = "ALF"
    server = server_faker(instrument_name)
    run_field, user_input = "experiment_identifier", "1234567"
    search_results = server.search(instrument_name, run_field, user_input)

    assert search_results.run_count == 2


def test_search_by_title_search_across_all_journals(server_faker):
    instrument_name = "ALF"
    server = server_faker(instrument_name)
    run_field, user_input = "title", "MnSi"
    search_results = server.search(instrument_name, run_field, user_input)

    assert search_results.run_count == 3


def test_search_by_run_number_search_across_all_journals(server_faker):
    instrument_name = "ALF"
    server = server_faker(instrument_name)
    run_field, user_input = "run_number", "83898-85424"
    search_results = server.search(instrument_name, run_field, user_input)

    assert search_results.run_count == 4


# private
def _fake_instrument_journallist_url(instrument_name: str) -> str:
    return FAKE_SERVER_ADDRESS + f"/{instrument_name.lower()}/journal_main.xml"


def _fake_instrument_journal_url(instrument_name: str, journal_filename: str) -> str:
    return FAKE_SERVER_ADDRESS + f"/{instrument_name.lower()}/{journal_filename}"
