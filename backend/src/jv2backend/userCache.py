# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from platformdirs import user_data_dir
from pathlib import Path
import hashlib
import datetime
import logging

# App and author names to identify user data
CACHE_APP_NAME = "JournalViewer2"
CACHE_APP_AUTHORS = "TeamJournalViewer"
_CACHE_ACTIVATED = False

def _item_hash(source_id: str, data_name: str) -> str:
    """Return the hash for the supplied source id and data name"""
    return hashlib.sha256(str(source_id + "_" + data_name).encode("utf-8")).hexdigest()

def _cache_dir() -> str:
    """Return the cache directory location"""
    return user_data_dir(CACHE_APP_NAME, CACHE_APP_AUTHORS)

def _cache_file(source_id: str, data_name: str) -> Path:
    """Return the full path to a cache file in the user space.
    The filename is a hash of the source id and data name provided.
    """
    return Path(_cache_dir()) / _item_hash(source_id, data_name)

def _cache_file_mtime(source_id: str, data_name: str) -> Path:
    """Return the full path to a cache file in the user space.
    The filename is a hash of the source id and data name provided.
    """
    return Path(_cache_dir()) / (_item_hash(source_id, data_name) + ".mtime")

def initialise() -> None:
    """Perform basic set up of the cache, principally making sure the target
    user data directory exists before we try and use it"""
    file = Path(_cache_dir())
    file.mkdir(parents=True, exist_ok=True)

    # Activate the cache
    global _CACHE_ACTIVATED
    _CACHE_ACTIVATED = True

def put_data(source_id: str, data_name: str, data: str,
             mtime: datetime.datetime = None) -> None:
    """Create a user data file with a hashed name reflecting the supplied
    source identifier and data name. Typically the data name will be a
    journal filename.
    """
    if not _CACHE_ACTIVATED:
        return

    # Write main file data
    try:
        with open(_cache_file(source_id, data_name), "wb") as file:
            file.write(bytes(data, "utf-8"))
    except Exception as exc:
        logging.error(f"Couldn't write cache file "
                      f"{_cache_file(source_id, data_name)} for "
                      f"({source_id}, {data_name}): {str(exc)}")

    # Write mtime if supplied
    if mtime is not None:
        try:
            with open(_cache_file_mtime(source_id, data_name), "wb") as file:
                file.write(bytes(str(mtime), "utf-8"))
        except Exception as exc:
            logging.error(f"Couldn't write cache file mtime "
                          f"{_cache_file_mtime(source_id, data_name)} for "
                          f"({source_id}, {data_name}): {str(exc)}")
