# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from platformdirs import user_data_dir
from pathlib import Path
import os.path
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

    logging.debug(f"Putting cache data for '{source_id}' / '{data_name}' "
                  f" => '{_cache_file(source_id, data_name)}")

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
            logging.error(f"Couldn't write cached mtime "
                          f"{_cache_file_mtime(source_id, data_name)} for "
                          f"({source_id}, {data_name}): {str(exc)}")


def has_data(source_id: str, data_name: str) -> bool:
    """Return whether the specified data exists in the user cache."""
    if not _CACHE_ACTIVATED:
        return False

    logging.debug(f"Checking cache data for '{source_id}' / '{data_name}' "
                  f" => '{_cache_file(source_id, data_name)}")

    return os.path.exists(_cache_file(source_id, data_name))


def has_mtime(source_id: str, data_name: str) -> bool:
    """Return whether the specified mtime exists in the user cache."""
    if not _CACHE_ACTIVATED:
        return False

    return os.path.exists(_cache_file_mtime(source_id, data_name))


def get_data(source_id: str, data_name: str) -> (bytes, datetime.datetime):
    """Retrieve the data and associated mtime (if available)"""
    data = bytes
    mtime = datetime.datetime

    logging.debug(f"Getting cache data for '{source_id}' / '{data_name}' "
                  f" => '{_cache_file(source_id, data_name)}")

    # Get data file
    try:
        with open(_cache_file(source_id, data_name), "rb") as data_file:
            data = data_file.read()
    except Exception as exc:
        logging.error(f"Couldn't read cache file "
                      f"{_cache_file(source_id, data_name)} for "
                      f"({source_id}, {data_name}): {str(exc)}")

    # Get mtime
    if os.path.exists(_cache_file_mtime(source_id, data_name)):
        mtime = get_mtime(source_id, data_name)

    return data, mtime


def get_mtime(source_id: str, data_name: str) -> datetime.datetime:
    """Retrieve the specified mtime"""
    try:
        with open(_cache_file_mtime(source_id,
                                    data_name), "rb") as mtime_file:
            mtime_data = mtime_file.read()
            return datetime.datetime.fromisoformat(mtime_data.decode("utf-8"))
    except Exception as exc:
        logging.warning(f"Couldn't read cached mtime "
                        f"{_cache_file_mtime(source_id, data_name)} for "
                        f"({source_id}, {data_name}): {str(exc)}")


def get_file_size(source_id: str, data_name: str) -> int:
    """Retrieve the file size of the cached data on disk"""
    try:
        return os.path.getsize(_cache_file(source_id, data_name))
    except Exception as exc:
        logging.warning(f"Couldn't read cached file size of "
                        f"{_cache_file(source_id, data_name)} for "
                        f"({source_id}, {data_name}): {str(exc)}")
