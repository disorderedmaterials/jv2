# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from jv2backend.journalserver import JournalServerURLBuilder

class ISISJournalServerURLBuilder(JournalServerURLBuilder):
    """Build Urls to access resources on the server"""

    def __init__(self, base_url: str) -> None:
        """
        :param base_url: Base url to access the ISIS Journal Server
        """
        super().__init__()
        self._base_url = base_url

    def indexfile_url(self, instrument_name: str) -> str:
        return self._base_url + f"/ndx{instrument_name.lower()}/journal_main.xml"
