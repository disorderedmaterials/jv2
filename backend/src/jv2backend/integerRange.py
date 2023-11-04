# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

class IntegerRange:
    """Defines an inclusive integer range"""

    def __init__(self, first: int = None, last: int = None):
        self._first = first
        self._last = last

    @classmethod
    def from_string(cls, text: str):
        """Create a range from the specified text"""
        if "-" in text:
            bits = list(filter(None, text.split("-")))
            if len(bits) == 1:
                istart, iend = int(bits[0].strip()), int(bits[0].strip())
            elif len(bits) == 2:
                istart, iend = int(bits[0].strip()), int(bits[1].strip())
            else:
                raise ValueError(f"The string '{text}' cannot be converted "
                                 f"to a range")

            if iend < istart:
                return cls(iend, istart)
            else:
                return cls(istart, iend)
        else:
            return cls(int(text.strip(), int(text.strip())))

    @property
    def firat(self) -> int:
        return self._first

    @property
    def last(self) -> int:
        return self._last

    def __contains__(self, value: int) -> bool:
        """Return whether the range contains the specified value"""
        return self._first <= value <= self._last

    def extend(self, value: int) -> bool:
        """Attempt to extend the current range with the supplied number.
        We will only do so if it results in another continuous range.
        """
        if (value+1) == self._first:
            self._first = value
            return True
        elif (value-1) == self._last:
            self._last = value
            return True

        return False
