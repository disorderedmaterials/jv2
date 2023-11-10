# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from jv2backend.requestData import RequestData, InvalidRequest
from jv2backend.journalLibrary import JournalLibrary
from jv2backend.journalCollection import JournalCollection
from jv2backend.journal import Journal, SourceType
import datetime
import pytest

# Test Data
POST_SOURCE_ID = "/my/source/id"
POST_JOURNAL_SOURCE_TYPE = "Network"
POST_JOURNAL_ROOT_URL = "/fake/root/url"
POST_JOURNAL_FILENAME = "fakeFilename.xml"
POST_RUN_DATA_ROOT_URL = "/run/data/fake/root/url"


def test_invalid_constructor():
    with pytest.raises(InvalidRequest) as exc:
        data = RequestData({})
    assert str(exc.value) == "No source ID provided in request."


def test_basic_constructor():
    post_data = {
        "sourceID": POST_SOURCE_ID,
        "sourceType": POST_JOURNAL_SOURCE_TYPE
    }

    try:
        data = RequestData(post_data)
    except Exception as exc:
        pytest.fail(f"Unexpected exception: {exc}")


def test_library_key_generation():
    post_data = {
        "sourceID": POST_SOURCE_ID,
        "sourceType": POST_JOURNAL_SOURCE_TYPE
    }

    try:
        data = RequestData(post_data)
    except Exception as exc:
        pytest.fail(f"Unexpected exception: {exc}")

    assert data.library_key() == POST_SOURCE_ID

    post_data["instrument"] = "bananas"

    try:
        data = RequestData(post_data)
    except Exception as exc:
        pytest.fail(f"Unexpected exception: {exc}")

    assert data.library_key() == POST_SOURCE_ID + "/bananas"


def test_journal_file_required_and_provided():
    post_data = {
        "sourceID": POST_SOURCE_ID,
        "sourceType": POST_JOURNAL_SOURCE_TYPE,
        "journalRootUrl": POST_JOURNAL_ROOT_URL,
        "journalFilename": POST_JOURNAL_FILENAME
    }

    try:
        data = RequestData(post_data, require_journal_file=True)
    except Exception as exc:
        pytest.fail(f"Unexpected exception: {exc}")

    assert data.journal_root_url == POST_JOURNAL_ROOT_URL
    assert data.journal_filename == POST_JOURNAL_FILENAME
    assert data.journal_file_url() == POST_JOURNAL_ROOT_URL + "/" + POST_JOURNAL_FILENAME


def test_journal_file_required_but_not_provided():
    post_data = {
        "sourceID": POST_SOURCE_ID,
        "sourceType": POST_JOURNAL_SOURCE_TYPE,
        "journalRootUrl": POST_JOURNAL_ROOT_URL
    }

    with pytest.raises(InvalidRequest) as exc:
        data = RequestData(post_data, require_journal_file=True)
    assert str(exc.value) == "No journal filename provided in request."


def test_run_data_directory_required_and_provided():
    post_data = {
        "sourceID": POST_SOURCE_ID,
        "sourceType": POST_JOURNAL_SOURCE_TYPE,
        "runDataRootUrl": POST_RUN_DATA_ROOT_URL
    }

    try:
        data = RequestData(post_data, require_data_directory=True)
    except Exception as exc:
        pytest.fail(f"Unexpected exception: {exc}")

    assert data.run_data_root_url == POST_RUN_DATA_ROOT_URL
