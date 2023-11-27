# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Defines the Flask endpoints that only access NeXuS information"""
import logging

from flask import Flask, jsonify, request, make_response
from flask.wrappers import Response as FlaskResponse
from jv2backend.classes.requestData import RequestData, InvalidRequest
import jv2backend.main.nexus
import jv2backend.main.library
import json


def add_routes(
    app: Flask,
    journalLibrary: jv2backend.main.library.JournalLibrary
) -> Flask:
    """Add routes to the given Flask application."""

    @app.post("/runData/nexus/getLogValues")
    def get_log_values() -> FlaskResponse:
        """Return the available log fields for one or more run numbers.

        The POST data should contain:
         runNumbers: Array of run numbers to probe for SE log values

        :return: A JSON response with the list of full paths to log fields
        """
        try:
            post_data = RequestData(request.json,
                                    require_run_numbers=True)
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        # Check for valid collection
        if post_data.library_key() not in journalLibrary:
            return make_response(
                jsonify({"Error": f"Collection {post_data.library_key()} "
                                  f"does not exist."}), 200
            )
        collection = journalLibrary[post_data.library_key()]

        # Locate data files for the specified run numbers in the collection
        data_files = collection.locate_data_files(post_data.run_numbers)
        logging.debug(data_files)
        logpaths = []
        for run in data_files:
            if data_files[run] is None:
                return make_response(
                    jsonify({"Error": f"Unable to find data file for run "
                             "{run}"}), 200
                )
            logpaths.extend(jv2backend.main.nexus.logpaths_from_path(data_files[run]))

        return make_response(jsonify(logpaths), 200)

    @app.post("/runData/nexus/getLogValueData")
    def get_log_value_data() -> FlaskResponse:
        """Return log value data specified for one or more run numbers.

        The POST data should contain:
         runNumbers: Array of run numbers to probe for SE log values
           logValue: Log value to retrieve

        :return: A list of the log data
        """
        try:
            post_data = RequestData(request.json,
                                    require_run_numbers=True,
                                    require_parameters="logValue")
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        # Check for valid collection
        if post_data.library_key() not in journalLibrary:
            return make_response(
                jsonify({"Error": f"Collection {post_data.library_key()} "
                                  f"does not exist."}), 200
            )
        collection = journalLibrary[post_data.library_key()]

        # Locate data files for the specified run numbers in the collection
        data_files = collection.locate_data_files(post_data.run_numbers)

        # Retrieve the log value data
        log_value = post_data.parameter("logValue")
        log_value_data = {}
        for run in data_files:
            if data_files[run] is None:
                return make_response(
                    jsonify({"Error": f"Unable to find data file for run "
                                      "{run}"}), 200
                )

            run_data = {}
            nxsfile, first_group = jv2backend.main.nexus.open_at(data_files[run], 0)

            run_data["runNumber"] = str(run)
            run_data["timeRange"] = [jv2backend.main.nexus.timerange(first_group)]
            run_data["data"] = jv2backend.main.nexus.logvalues(first_group[log_value])

            log_value_data[run] = run_data

        return make_response(jsonify(
            {
                "logValue": log_value,
                "runNumbers":  post_data.run_numbers,
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
            post_data = RequestData(request.json,
                                    require_run_numbers=True,
                                    require_parameters="spectrumType")
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        # Check for valid collection
        if post_data.library_key() not in journalLibrary:
            return make_response(
                jsonify({"Error": f"Collection {post_data.library_key()} "
                                  f"does not exist."}), 200
            )
        collection = journalLibrary[post_data.library_key()]

        # Locate data file for the specified run number in the collection
        run_number = post_data.run_numbers[0]
        data_file = collection.locate_data_file(run_number)
        if data_file is None:
            return make_response(
                jsonify({"Error": f"Unable to find data file for run "
                                  "{run}"}), 200
            )

        spectrum_type = post_data.parameter("spectrumType")
        if spectrum_type == "monitor":
            return make_response(
                jsonify(jv2backend.main.nexus.get_monitor_count(data_file)),
                200)
        elif spectrum_type == "detector":
            return make_response(
                jsonify(jv2backend.main.nexus.get_detector_count(data_file)),
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
            post_data = RequestData(request.json,
                                    require_run_numbers=True,
                                    require_parameters="spectrumId,spectrumType")
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        # Check for valid collection
        if post_data.library_key() not in journalLibrary:
            return make_response(
                jsonify({"Error": f"Collection {post_data.library_key()} "
                                  f"does not exist."}), 200
            )
        collection = journalLibrary[post_data.library_key()]

        # Locate data files for the specified run numbers in the collection
        data_files = collection.locate_data_files(post_data.run_numbers)

        # Get request parameters
        spectrum_id = int(post_data.parameter("spectrumId"))
        spectrum_type = post_data.parameter("spectrumType")

        # first entry matches sata expectation of the frontend
        spectra = [[post_data.run_numbers, spectrum_id, spectrum_type]]
        for run in data_files:
            if run is None:
                continue
            if spectrum_type == "monitor":
                spectra.append(jv2backend.main.nexus.get_monitor_spectrum(
                    data_files[run],
                    spectrum_id)
                )
            elif spectrum_type == "detector":
                spectra.append(jv2backend.main.nexus.get_detector_spectrum(
                    data_files[run],
                    spectrum_id)
                )

        return make_response(jsonify(spectra), 200)

    @app.post("/runData/nexus/getDetectorAnalysis")
    def get_detector_analysis() -> FlaskResponse:
        """Determine the number of spectra with non-zero signal values

        The POST data should contain:
          runNumber: Run number to get analysis for

        :return: A string of the form "count(non_zero)/count(all_spectra)"
        """
        try:
            post_data = RequestData(request.json,
                                    require_run_numbers=True)
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        # Check for valid collection
        if post_data.library_key() not in journalLibrary:
            return make_response(
                jsonify({"Error": f"Collection {post_data.library_key()} "
                                  f"does not exist."}), 200
            )
        collection = journalLibrary[post_data.library_key()]

        # Locate data file for the specified run number in the collection
        run_number = post_data.run_numbers[0]
        data_file = collection.locate_data_file(run_number)
        if data_file is None:
            return make_response(
                jsonify({"Error": f"Unable to find data file for run "
                                  "{run}"}), 200
            )

        return make_response(
            jv2backend.main.nexus.nonzero_spectra_ratio(data_file),
            200
        )

    # ------------------------ End Routes -------------------------

    return app
