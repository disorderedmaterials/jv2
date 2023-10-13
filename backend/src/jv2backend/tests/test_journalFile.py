# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from jv2backend.journalClasses import JournalFile


def test_default_constructed_empty():
    index = JournalFile()

    assert len(index) == 0


def test_append_accepts_given_text():
    index = JournalFile()
    name = "journal_21_1.xml"

    index.append(name)

    assert index[0] == name
