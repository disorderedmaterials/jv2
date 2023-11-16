# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import jv2backend.utils


A = "alpha"
B = "BETA"
C = "gamma"
D = "/"
E = ""


def test_url_join_result():
    assert jv2backend.utils.url_join(A, B, C) == A + "/" + B + "/" + C


def test_url_join_single_argument():
    assert jv2backend.utils.url_join(A) == A


def test_url_join_handles_empty_strings_correctly():
    assert jv2backend.utils.url_join(A, E, C) == A + "/" + C


def test_url_join_handles_None_correctly():
    assert jv2backend.utils.url_join(A, None, C) == A + "/" + C
    assert jv2backend.utils.url_join(None, A, C) == A + "/" + C
    assert jv2backend.utils.url_join(A, C, None) == A + "/" + C
    assert jv2backend.utils.url_join(A, None, None) == A
