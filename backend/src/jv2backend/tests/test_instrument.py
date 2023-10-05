# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors
from jv2backend.instrument import Instrument


def test_instrument_creation_requires_full_name():
    fake_instrument_name = "FAKE2D"
    instrument = Instrument(fake_instrument_name)

    assert instrument.name == fake_instrument_name
