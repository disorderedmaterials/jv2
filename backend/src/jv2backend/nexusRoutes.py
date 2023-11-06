# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Defines the Flask endpoints that only access NeXuS information"""
import logging

from flask import Flask, jsonify, request, make_response
from jv2backend.requestData import RequestData, InvalidRequest
from jv2backend.utils import json_response
import jv2backend.nexus


def add_routes(
    app: Flask,
    journalLibrary: jv2backend.journalLibrary.JournalLibrary
) -> Flask:
    """Add routes to the given Flask application."""

    @app.post("/runData/nexus/getLogValues")
    def getLogValues():
        """Return the available log fields for one or more run numbers.

        The POST data should contain:
        journalRoot: The root network or disk location for the journals
          directory: The directory in journalRoot containing the journals
         runNumbers: Array of run numbers to probe for SE log values

        :return: A JSON response with the list of full paths to log fields
        """
        try:
            postData = RequestData(request.json,
                                   require_run_numbers=True)
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        # Check for valid collection
        if postData.library_key() not in journalLibrary:
            return make_response(
                jsonify({"Error": f"Collection {postData.library_key()} "
                                  f"does not exist."}), 200
            )
        collection = journalLibrary[postData.library_key()]

        # Locate data files for the specified run numbers in the collection
        dataFiles = collection.locate_data_files(postData.run_numbers)
        logging.debug(dataFiles)
        logpaths = []
        for run in dataFiles:
            if dataFiles[run] is None:
                return make_response(
                    jsonify({"Error": f"Unable to find data file for run "
                             "{run}"}), 200
                )
            logpaths.extend(jv2backend.nexus.logpaths_from_path(dataFiles[run]))

        return json_response(logpaths)

    @app.post("/runData/nexus/getLogValueData")
    def getLogValueData():
        """Return log value data specified for one or more run numbers.

        The POST data should contain:
        journalRoot: The root network or disk location for the journals
          directory: The directory in journalRoot containing the journals
         runNumbers: Array of run numbers to probe for SE log values
           logValue: Log value to retrieve

        :return: A list of the log data
        """
        try:
            postData = RequestData(request.json,
                                   require_run_numbers=True,
                                   require_parameter="logValue")
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        # Check for valid collection
        if postData.library_key() not in journalLibrary:
            return make_response(
                jsonify({"Error": f"Collection {postData.library_key()} "
                                  f"does not exist."}), 200
            )
        collection = journalLibrary[postData.library_key()]

        # Locate data files for the specified run numbers in the collection
        dataFiles = collection.locate_data_files(postData.run_numbers)

        # Retrieve the log value data
        log_value_data = {}
        for run in dataFiles:
            if dataFiles[run] is None:
                return make_response(
                    jsonify({"Error": f"Unable to find data file for run "
                                      "{run}"}), 200
                )

            runData = {}
            nxsfile, first_group = jv2backend.nexus.open_at(dataFiles[run], 0)

            runData["runNumber"] = str(run)
            runData["timeRange"] = [jv2backend.nexus.timerange(first_group)]
            runData["data"] = jv2backend.nexus.logvalues(first_group[postData.parameter])

            log_value_data[run] = runData

        # Construct the final return object
        result = {}
        result["logValue"] = postData.parameter
        result["runNumbers"] = postData.run_numbers
        result["data"] = log_value_data

        return json_response(result)

    @app.post("/runData/nexus/getSpectrumRange")
    def getSpectrumRange():
        """Return the number of detector spectra for the run.

        The POST data should contain:
            journalRoot: The root network or disk location for the journals
          directory: The directory in journalRoot containing the journals
          runNumber: Run number to probe for detector count

        :return: The number of available detectors
        """
        try:
            postData = RequestData(request.json,
                                   require_run_numbers=True)
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        # Check for valid collection
        if postData.library_key() not in journalLibrary:
            return make_response(
                jsonify({"Error": f"Collection {postData.library_key()} "
                                  f"does not exist."}), 200
            )
        collection = journalLibrary[postData.library_key()]

        # Locate data file for the specified run number in the collection
        run_number = postData.run_numbers[0]
        dataFile = collection.locate_data_file(run_number)
        if dataFile is None:
            return make_response(
                jsonify({"Error": f"Unable to find data file for run "
                                  "{run}"}), 200
            )

        return str(jv2backend.nexus.spectra_count(dataFile))

    @app.post("/runData/nexus/getMonitorRange")
    def getMonitorRange():
        """Return the number of monitor spectra for the run.

        The POST data should contain:
        journalRoot: The root network or disk location for the journals
          directory: The directory in journalRoot containing the journals
          runNumber: Run number to probe for monitor count

        :return: The number of available monitors
        """
        try:
            postData = RequestData(request.json,
                                   require_run_numbers=True)
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        # Check for valid collection
        if postData.library_key() not in journalLibrary:
            return make_response(
                jsonify({"Error": f"Collection {postData.library_key()} "
                                  f"does not exist."}), 200
            )
        collection = journalLibrary[postData.library_key()]

        # Locate data file for the specified run number in the collection
        run_number = postData.run_numbers[0]
        dataFile = collection.locate_data_file(run_number)
        if dataFile is None:
            return make_response(
                jsonify({"Error": f"Unable to find data file for run "
                                  "{run}"}), 200
            )

        return str(jv2backend.nexus.monitor_count(dataFile))

    @app.post("/runData/nexus/getSpectrum")
    def getSpectrum():
        """Return detector spectrum for one or more run numbers.

        The POST data should contain:
        journalRoot: The root network or disk location for the journals
          directory: The directory in journalRoot containing the journals
         runNumbers: Array of run numbers to work on
         spectrumId: Target detector spectrum to return

        :return: A list of the detector spectra
        """
        try:
            postData = RequestData(request.json,
                                   require_run_numbers=True,
                                   require_parameter="spectrumId")
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        # Check for valid collection
        if postData.library_key() not in journalLibrary:
            return make_response(
                jsonify({"Error": f"Collection {postData.library_key()} "
                                  f"does not exist."}), 200
            )
        collection = journalLibrary[postData.library_key()]

        # Locate data files for the specified run numbers in the collection
        dataFiles = collection.locate_data_files(postData.run_numbers)

        # first entry matches sata expectation of the frontend
        spectra = [[postData.run_numbers, postData.parameter, "detector"]]
        for run in dataFiles:
            if run is not None:
                spectra.append(jv2backend.nexus.spectrum(dataFiles[run],
                                            int(postData.parameter)))

        return json_response(spectra)

    @app.post("/runData/nexus/getMonitorSpectrum")
    def getMonitorSpectrum():
        """Return monitor spectrum for one or more run numbers.

        The POST data should contain:
        journalRoot: The root network or disk location for the journals
          directory: The directory in journalRoot containing the journals
         runNumbers: Array of run numbers to work on
         spectrumId: Target monitor spectrum to return

        :return: A list of the monitor spectra
        """
        try:
            postData = RequestData(request.json,
                                   require_run_numbers=True,
                                   require_parameter="spectrumId")
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        # Check for valid collection
        if postData.library_key() not in journalLibrary:
            return make_response(
                jsonify({"Error": f"Collection {postData.library_key()} "
                                  f"does not exist."}), 200
            )
        collection = journalLibrary[postData.library_key()]

        # Locate data files for the specified run numbers in the collection
        dataFiles = collection.locate_data_files(postData.run_numbers)

        # first entry matches data expectation of the frontend
        spectra = [[postData.run_numbers, postData.parameter, "monitor"]]
        for run in dataFiles:
            if run is not None:
                spectra.append(jv2backend.nexus.monitor_spectrum(dataFiles[run],
                                                    int(postData.parameter)))

        return json_response(spectra)

    @app.post("/runData/nexus/getDetectorAnalysis")
    def getDetectorAnalysis():
        """Determine the number of spectra with non-zero signal values

        The POST data should contain:
        journalRoot: The root network or disk location for the journals
          directory: The directory in journalRoot containing the journals
          runNumber: Run number to get analysis for

        :return: A string of the form "count(non_zero)/count(all_spectra)"
        """
        try:
            postData = RequestData(request.json,
                                   require_run_numbers=True)
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        # Check for valid collection
        if postData.library_key() not in journalLibrary:
            return make_response(
                jsonify({"Error": f"Collection {postData.library_key()} "
                                  f"does not exist."}), 200
            )
        collection = journalLibrary[postData.library_key()]

        # Locate data file for the specified run number in the collection
        run_number = postData.run_numbers[0]
        dataFile = collection.locate_data_file(run_number)
        if dataFile is None:
            return make_response(
                jsonify({"Error": f"Unable to find data file for run "
                                  "{run}"}), 200
            )

        return jv2backend.nexus.nonzero_spectra_ratio(dataFile)

    # ------------------------ End Routes -------------------------

    return app
