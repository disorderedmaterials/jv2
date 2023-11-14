# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Defines the Flask endpoints that only access NeXuS information"""
import logging

from flask import Flask, jsonify, request, make_response
from flask.wrappers import Response as FlaskResponse
from jv2backend.requestData import RequestData, InvalidRequest
from jv2backend.utils import json_response
import jv2backend.nexus
import json


def add_routes(
    app: Flask,
    journalLibrary: jv2backend.journalLibrary.JournalLibrary
) -> Flask:
    """Add routes to the given Flask application."""

    @app.post("/runData/nexus/getLogValues")
    def getLogValues() -> FlaskResponse:
        """Return the available log fields for one or more run numbers.

        The POST data should contain:
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

        return make_response(jsonify(logpaths), 200)

    @app.post("/runData/nexus/getLogValueData")
    def getLogValueData() -> FlaskResponse:
        """Return log value data specified for one or more run numbers.

        The POST data should contain:
         runNumbers: Array of run numbers to probe for SE log values
           logValue: Log value to retrieve

        :return: A list of the log data
        """
        try:
            postData = RequestData(request.json,
                                   require_run_numbers=True,
                                   require_parameters="logValue")
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
        log_value = postData.parameter("logValue")
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
            runData["data"] = jv2backend.nexus.logvalues(first_group[log_value])

            log_value_data[run] = runData

        return make_response(jsonify(
            {
                "logValue": log_value,
                "runNumbers":  postData.run_numbers,
                "data": log_value_data
            }
        ), 200)

    @app.post("/runData/nexus/getSpectrumCount")
    def get_spectrum_count() -> FlaskResponse:
        """Return the number of spectra - monitor or detector - for the run.

        The POST data should contain:
             runNumber: Run number to probe for spectrum count
          spectrumType: Either "monitor" or "detector"

        :return: The number of available detectors
        """
        try:
            postData = RequestData(request.json,
                                   require_run_numbers=True,
                                   require_parameters="spectrumType")
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

        spectrum_type = postData.parameter("spectrumType")
        if spectrum_type == "monitor":
            return make_response(
                jsonify(jv2backend.nexus.get_monitor_count(dataFile)),
                200)
        elif spectrum_type == "detector":
            return make_response(
                jsonify(jv2backend.nexus.get_detector_count(dataFile)),
                200)
        else:
            return make_response(
                json.dumps({"Error": f"Unrecognised spectrum type "
                                     f"'{spectrum_type}'"}),
                200)

    @app.post("/runData/nexus/getSpectrum")
    def get_spectrum() -> FlaskResponse:
        """Return spectrum for one or more run numbers.

        The POST data should contain:
           runNumbers: Array of run numbers to work on
           spectrumId: Target spectrum index to return
         spectrumType: Spectrum type to return - either monitor or detector

        :return: A list of the detector spectra
        """
        try:
            postData = RequestData(request.json,
                                   require_run_numbers=True,
                                   require_parameters="spectrumId,spectrumType")
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

        # Get request parameters
        spectrum_id = int(postData.parameter("spectrumId"))
        spectrum_type = postData.parameter("spectrumType")

        # first entry matches sata expectation of the frontend
        spectra = [[postData.run_numbers, spectrum_id, spectrum_type]]
        for run in dataFiles:
            if run is None:
                continue
            if spectrum_type == "monitor":
                spectra.append(jv2backend.nexus.get_monitor_spectrum(
                    dataFiles[run],
                    spectrum_id)
                )
            elif spectrum_type == "detector":
                spectra.append(jv2backend.nexus.get_detector_spectrum(
                    dataFiles[run],
                    spectrum_id)
                )

        return make_response(jsonify(spectra), 200)


    @app.post("/runData/nexus/getDetectorAnalysis")
    def getDetectorAnalysis() -> FlaskResponse:
        """Determine the number of spectra with non-zero signal values

        The POST data should contain:
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

        return make_response(
            jv2backend.nexus.nonzero_spectra_ratio(dataFile),
            200
        )

    # ------------------------ End Routes -------------------------

    return app
