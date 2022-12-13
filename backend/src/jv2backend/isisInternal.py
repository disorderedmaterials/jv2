# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin and T. Youngs

from flask import Flask
from flask import jsonify
from flask import request

from urllib.request import urlopen
import lxml.etree as ET
from xml.etree.ElementTree import parse
from xml.etree.ElementTree import fromstring

from ast import literal_eval

from datetime import datetime
from datetime import timedelta

import requests

from . import nexusInteraction

from os import path

# Constants
dataLocation = "http://data.isis.rl.ac.uk/journals/"
localSource = ""




def create_app():
    """Create and return the flask app """
    global app
    app = Flask(__name__)
    return app


def shutdown_server():
    """Shutdown a local flask server if it is in use.
    If an external WSGI server is used this raises a warning in the
    WSGI server will log
    """
    serverShutdownFunction = request.environ.get('werkzeug.server.shutdown')
    if serverShutdownFunction is None:
        app.logger.info('Local development server not active. '
                        'Shutdown request ignored. '
                        'You most likely need to kill the external '
                        'WSGI process.')
    else:
        serverShutdownFunction()


# Global Flask application instance
app = create_app()

# Set local source
@app.route('/setLocalSource/<inLocalSource>')
def setLocalSource(inLocalSource):
    global localSource
    localSource = inLocalSource.replace(";", "/")
    print("local source: " + localSource)
    return jsonify("success")


# clear local source
@app.route('/clearLocalSource')
def clearLocalSource():
    global localSource
    localSource = ""
    print("local source: " + localSource)
    return jsonify("success")

# validate local source


@app.route('/validateLocalSource')
def validateLocalSource():
    global localSource
    valid = path.isdir(localSource)
    if valid or localSource == "":
        response = ""
    else:
        response = "Local source not valid"
    return jsonify(response)

# Get nexus file fields


@app.route('/getNexusFields/<instrument>/<cycles>/<runs>')
def getNexusFields(instrument, cycles, runs):
    print("CYCLES: " + cycles)
    runFields = nexusInteraction.runFields(instrument, cycles, runs)
    return jsonify(runFields)

# Get all log data from nexus field


@app.route('/getNexusData/<instrument>/<cycles>/<runs>/<fields>')
def getNexusData(instrument, cycles, runs, fields):
    fieldData = nexusInteraction.fieldData(instrument, cycles, runs, fields)
    return jsonify(fieldData)

# Get instrument cycles


@app.route('/getCycles/<instrument>')
def getCycles(instrument):
    global lastModified_
    url = dataLocation + 'ndx'
    url += instrument+'/journal_main.xml'
    try:
        response = urlopen(url)
    except Exception:
        return jsonify({"response": "ERR. url not found"})
    lastModified_ = response.info().get('Last-Modified')
    lastModified_ = datetime.strptime(
        lastModified_, "%a, %d %b %Y %H:%M:%S %Z")
    print("lastModified_ set as: ")
    print(lastModified_)
    tree = parse(response)
    root = tree.getroot()
    cycles = []
    for data in root:
        cycles.append(data.get('name'))

    return jsonify(cycles)

# Get cycle run data


@app.route('/getJournal/<instrument>/<cycle>')
def getJournal(instrument, cycle):
    global localSource
    try:
        with open(localSource + 'ndx' + instrument+'/'+cycle, "r") as file:
            root = fromstring(file.read())
            print("data from file")
    except Exception:
        if localSource != "":
            return jsonify("invalid source")
        url = dataLocation + 'ndx' + instrument+'/'+cycle
        print(f"url={url}")
        try:
            response = urlopen(url)
        except Exception:
            return jsonify({"response": "ERR. url not found"})
        tree = parse(response)
        root = tree.getroot()
        print("data from server")

    fields = []
    for run in root:
        runData = {}
        for data in run:
            dataId = data.tag.replace(
                '{http://definition.nexusformat.org/schema/3.0}', '')
            try:
                dataValue = data.text.strip()
            except Exception:
                dataValue = data.text
            # If value is valid date
            try:
                time = datetime.strptime(dataValue, "%Y-%m-%dT%H:%M:%S")
                today = datetime.now()
                if (today.date() == time.date()):
                    runData[dataId] = "Today at: " + time.strftime("%H:%M:%S")
                elif ((today + timedelta(-1)).date() == time.date()):
                    runData[dataId] = "Yesterday at: " + \
                        time.strftime("%H:%M:%S")
                else:
                    runData[dataId] = time.strftime("%d/%m/%Y %H:%M:%S")
            except Exception:
                # If header is duration then format
                if (dataId == "duration"):
                    dataValue = int(dataValue)
                    minutes = dataValue // 60
                    seconds = str(dataValue % 60).rjust(2, '0')
                    hours = str(minutes // 60).rjust(2, '0')
                    minutes = str(minutes % 60).rjust(2, '0')
                    runData[dataId] = hours + ":" + minutes + ":" + seconds
                else:
                    runData[dataId] = dataValue
        fields.append(runData)
    return jsonify(fields)

# Search all cycles


@app.route('/getAllJournals/<instrument>/<search>')
def getAllJournals(instrument, search):
    global localSource

    allFields = []
    nameSpace = {'data': 'http://definition.nexusformat.org/schema/3.0'}
    cycles = literal_eval(getCycles(instrument).get_data().decode())
    cycles.pop(0)

    startTime = datetime.now()

    for cycle in (cycles):
        print(instrument, " ", cycle)
        try:
            fileString = localSource + 'ndx' + instrument+'/'+str(cycle)
            with open(fileString, "r") as file:
                root = ET.fromstring(file.read())
        except Exception:
            url = dataLocation + 'ndx' + instrument+'/'+str(cycle)
            try:
                response = urlopen(url)
            except Exception:
                return jsonify({"response": "ERR. url not found"})
            tree = ET.parse(response)
            root = tree.getroot()

        fields = []
        """
        foundElems = root.findall
        ("./data:NXentry/[data:user_name='"+search+"']", nameSpace)
        """
        print("TEST " + search.lower())
        path = "//*[contains(translate(data:user_name/text(), " + \
            "'ABCDEFGHIJKLMNOPQRSTUVWXYZ', " + \
            "'abcdefghijklmnopqrstuvwxyz'),'" + \
            search.lower()+"')]"
        foundElems = root.xpath(path, namespaces=nameSpace)
        for element in foundElems:
            runData = {}
            for data in element:
                dataId = data.tag.replace(
                    '{http://definition.nexusformat.org/schema/3.0}', '')
                try:
                    dataValue = data.text.strip()
                except Exception:
                    dataValue = data.text
                # If value is valid date
                try:
                    time = datetime.strptime(dataValue, "%Y-%m-%dT%H:%M:%S")
                    today = datetime.now()
                    if (today.date() == time.date()):
                        runData[dataId] = "Today at: " + \
                            time.strftime("%H:%M:%S")
                    elif ((today + timedelta(-1)).date() == time.date()):
                        runData[dataId] = "Yesterday at: " + \
                            time.strftime("%H:%M:%S")
                    else:
                        runData[dataId] = time.strftime("%d/%m/%Y %H:%M:%S")
                except Exception:
                    # If header is duration then format
                    if (dataId == "duration"):
                        dataValue = int(dataValue)
                        minutes = dataValue // 60
                        seconds = str(dataValue % 60).rjust(2, '0')
                        hours = str(minutes // 60).rjust(2, '0')
                        minutes = str(minutes % 60).rjust(2, '0')
                        runData[dataId] = hours + ":" + minutes + ":" + seconds
                    else:
                        runData[dataId] = dataValue
            fields.append(runData)
        allFields += (fields)
        print(len(foundElems))

    endTime = datetime.now()
    print(endTime - startTime)
    return jsonify(allFields)

# Search all cycles with specified field


@app.route('/getAllJournals/<instrument>/<field>/<search>/<options>')
def getAllFieldJournals(instrument, field, search, options):
    global localSource
    print("\nMass search initiated w/: " +
          instrument + " " + field + " " + search + " " + options)
    allFields = []
    nameSpace = {'data': 'http://definition.nexusformat.org/schema/3.0'}
    cycles = literal_eval(getCycles(instrument).get_data().decode())
    cycles.pop(0)

    startTime = datetime.now()

    for cycle in (cycles):
        try:
            fileString = localSource + 'ndx' + instrument+'/'+str(cycle)
            with open(fileString, "r") as file:
                root = ET.fromstring(file.read())
        except Exception:
            url = dataLocation + 'ndx' + instrument+'/'+str(cycle)
            try:
                response = urlopen(url)
            except Exception:
                return jsonify({"response": "ERR. url not found"})
            tree = ET.parse(response)
            root = tree.getroot()

        fields = []
        if field == "run_number":
            values = search.split("-")
            path = "//*[data:run_number>"+values[0] + \
                " and data:run_number<"+values[1]+"]"
        elif field == "start_date":
            values = search.replace(";", "").split("-")
            dateAsNumber = \
                "number(translate(" + \
                "substring-before(data:start_time,'T'),'-',''))"
            path = \
                "//*["+dateAsNumber+" > "+values[0] + \
                " and "+dateAsNumber+" < " + values[1]+"]"
        else:
            if "caseSensitivity=true" in options:
                path = "//*[contains(data:"+field+",'"+search+"')]"
            else:
                path = "//*[contains(translate(data:"+field+"/text(), " + \
                    "'ABCDEFGHIJKLMNOPQRSTUVWXYZ', " + \
                    "'abcdefghijklmnopqrstuvwxyz'),'" + \
                    search.lower()+"')]"
        foundElems = root.xpath(path, namespaces=nameSpace)
        print(search)
        print(cycle + " FoundElems: " + str(len(foundElems)))
        for element in foundElems:
            runData = {}
            for data in element:
                dataId = data.tag.replace(
                    '{http://definition.nexusformat.org/schema/3.0}', '')
                try:
                    dataValue = data.text.strip()
                except Exception:
                    dataValue = data.text
                # If value is valid date
                try:
                    time = datetime.strptime(dataValue, "%Y-%m-%dT%H:%M:%S")
                    today = datetime.now()
                    if (today.date() == time.date()):
                        runData[dataId] = "Today at: " + \
                            time.strftime("%H:%M:%S")
                    elif ((today + timedelta(-1)).date() == time.date()):
                        runData[dataId] = "Yesterday at: " + \
                            time.strftime("%H:%M:%S")
                    else:
                        runData[dataId] = time.strftime("%d/%m/%Y %H:%M:%S")
                except Exception:
                    # If header is duration then format
                    if (dataId == "duration"):
                        dataValue = int(dataValue)
                        minutes = dataValue // 60
                        seconds = str(dataValue % 60).rjust(2, '0')
                        hours = str(minutes // 60).rjust(2, '0')
                        minutes = str(minutes % 60).rjust(2, '0')
                        runData[dataId] = hours + ":" + minutes + ":" + seconds
                    else:
                        runData[dataId] = dataValue
            fields.append(runData)
        allFields += (fields)

    endTime = datetime.now()
    print(endTime - startTime)
    return jsonify(allFields)

# Go to functionality


@app.route('/getGoToCycle/<instrument>/<search>')
def getGoToCycle(instrument, search):
    global localSource
    nameSpace = {'data': 'http://definition.nexusformat.org/schema/3.0'}
    cycles = literal_eval(getCycles(instrument).get_data().decode())
    cycles.pop(0)

    startTime = datetime.now()
    for cycle in (cycles):
        print(instrument, " ", cycle)

        try:
            fileString = localSource + 'ndx' + instrument+'/'+str(cycle)
            with open(fileString, "r") as file:
                root = ET.fromstring(file.read())
        except Exception:
            url = dataLocation + 'ndx' + instrument+'/'+str(cycle)
            try:
                response = urlopen(url)
            except Exception:
                return jsonify({"response": "ERR. url not found"})
            tree = ET.parse(response)
            root = tree.getroot()

        path = "//*[data:run_number="+search+"]"
        foundElems = root.xpath(path, namespaces=nameSpace)
        if len(foundElems) > 0:
            endTime = datetime.now()
            print(endTime - startTime)
            return cycle
        print(len(foundElems))

    endTime = datetime.now()
    print(endTime - startTime)
    return "Not Found"

# Get spectra data


@app.route('/getSpectrum/<instrument>/<cycle>/<runs>/<spectra>')
def getSpectrum(instrument, cycle, runs, spectra):
    data = nexusInteraction.getSpectrum(instrument, cycle, runs, spectra)
    return jsonify(data)


@app.route('/getMonSpectrum/<instrument>/<cycle>/<runs>/<monitor>')
def getMonSpectrum(instrument, cycle, runs, monitor):
    data = nexusInteraction.getMonSpectrum(instrument, cycle, runs, monitor)
    return jsonify(data)

# Get spectra range


@app.route('/getSpectrumRange/<instrument>/<cycle>/<runs>')
def getSpectrumRange(instrument, cycle, runs):
    data = nexusInteraction.getSpectrumRange(instrument, cycle, runs)
    return jsonify(data)

# Set archive mount


@app.route('/setRoot/<rootLocation>')
def setRoot(rootLocation):
    if rootLocation != "Default":
        nexusInteraction.setRoot(rootLocation)
    else:
        nexusInteraction.setRoot("Default")
    return jsonify("test")


@app.route('/getMonitorRange/<instrument>/<cycle>/<runs>')
def getMonitorRange(instrument, cycle, runs):
    data = nexusInteraction.getMonitorRange(instrument, cycle, runs)
    return jsonify(data)


@app.route('/getDetectorAnalysis/<instrument>/<cycle>/<run>')
def getDetectorAnalysis(instrument, cycle, run):
    data = nexusInteraction.detectorAnalysis(instrument, cycle, run)
    return jsonify(data)

# Get total MuAmps


@app.route('/getTotalMuAmps/<instrument>/<cycle>/<runs>')
def getTotalMuAmps(instrument, cycle, runs):
    global localSource
    try:
        with open(localSource + 'ndx' + instrument+'/'+cycle, "r") as file:
            root = fromstring(file.read())
    except Exception:
        url = dataLocation + 'ndx' + instrument+'/'+cycle
        try:
            response = urlopen(url)
        except Exception:
            return jsonify({"response": "ERR. url not found"})
        tree = parse(response)
        root = tree.getroot()

    ns = {'tag': 'http://definition.nexusformat.org/schema/3.0'}
    muAmps = ""
    for run in runs.split(";"):
        for runNo in root:
            if (runNo.find('tag:run_number', ns).text.strip() == run):
                muAmps += runNo.find('tag:proton_charge',
                                     ns).text.strip() + ";"
    return muAmps[:-1]

# Check for data modifications


@app.route('/pingCycle/<instrument>')
def pingCycle(instrument):
    global lastModified_
    url = dataLocation + 'ndx'
    url += instrument+'/journal_main.xml'
    lastModified = requests.head(url).headers['Last-Modified']
    lastModified = datetime.strptime(lastModified, "%a, %d %b %Y %H:%M:%S %Z")
    print(lastModified)
    print(lastModified_)
    if (lastModified > lastModified_):
        lastModified_ = lastModified
        response = urlopen(url)
        tree = parse(response)
        root = tree.getroot()
        return root[-1].get('name')
    return ("")


@app.route('/updateJournal/<instrument>/<cycle>/<run>')
def updateJournal(instrument, cycle):
    url = dataLocation + 'ndx'
    url += instrument+'/' + cycle
    try:
        response = urlopen(url)
    except Exception:
        return jsonify({"response": "ERR. url not found"})
    tree = parse(response)
    root = tree.getroot()
    ns = {'tag': 'http://definition.nexusformat.org/schema/3.0'}
    fields = []
    for run in root:
        if (run.find('tag:run_number', ns).text.strip() <= run):
            continue
        runData = {}
        for data in run:
            dataId = data.tag.replace(
                '{http://definition.nexusformat.org/schema/3.0}', '')
            try:
                dataValue = data.text.strip()
            except Exception:
                dataValue = data.text
            # If value is valid date
            try:
                time = datetime.strptime(dataValue, "%Y-%m-%dT%H:%M:%S")
                today = datetime.now()
                if (today.date() == time.date()):
                    runData[dataId] = "Today at: " + time.strftime("%H:%M:%S")
                elif ((today + timedelta(-1)).date() == time.date()):
                    runData[dataId] = "Yesterday at: " + \
                        time.strftime("%H:%M:%S")
                else:
                    runData[dataId] = time.strftime("%d/%m/%Y %H:%M:%S")
            except Exception:
                # If header is duration then format
                if (dataId == "duration"):
                    dataValue = int(dataValue)
                    minutes = dataValue // 60
                    seconds = str(dataValue % 60).rjust(2, '0')
                    hours = str(minutes // 60).rjust(2, '0')
                    minutes = str(minutes % 60).rjust(2, '0')
                    runData[dataId] = hours + ":" + minutes + ":" + seconds
                else:
                    runData[dataId] = dataValue
        fields.append(runData)
    return jsonify(fields)


# Close local server
@app.route('/shutdown', methods=['GET'])
def shutdown():
    shutdown_server()
    return jsonify({"response": "Server shut down"})


def main():
    """Start the flask development server with default settings"""
    #app = create_app()
    app.run()


# Start local development server when executed as a main routine
if __name__ == '__main__':
    main()
