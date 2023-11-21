# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from backend.jv2.classes.integerRange import IntegerRange


def test_basic_constructor():
    irange = IntegerRange(1, 10)
    _test_range(irange)


def test_construction_from_string():
    irange = IntegerRange.from_string("1-10")
    _test_range(irange)


def test_construction_from_string_reversed():
    irange = IntegerRange.from_string("10-1")
    _test_range(irange)


def test_construction_from_string_missing_first():
    irange = IntegerRange.from_string("-10")
    assert 10 in irange
    assert 9 not in irange
    assert 11 not in irange


def test_construction_from_string_missing_last():
    irange = IntegerRange.from_string("10-")
    assert 10 in irange
    assert 9 not in irange
    assert 11 not in irange


def test_extend_range():
    irange = IntegerRange(1, 8)

    assert not irange.extend(10)
    assert irange.extend(9)
    assert irange.extend(10)
    _test_range(irange)


# Private

def _test_range(range: IntegerRange):
    assert 1 in range
    assert 5 in range
    assert 0 not in range
    assert 100 not in range
