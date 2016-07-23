#!/usr/bin/env python
import flask
import pprint

application = flask.Flask(__name__)

def get_network_summary():
    lines = open('captures/latest/network.xml','r').readlines()
    machines = list()
    machine = dict()
    for line in lines:
        if line.startswith("</host>"):
            machines.append(machine)
            machine = dict()
        if line.startswith('<address addr="'):
            machine['ip'] = line.split('"')[1]
        if line.startswith('<hostname name="'):
            machine['hostname'] = line.split('"')[1]
    return machines

@application.route("/")
def hello():
    return flask.render_template('index.html', machines=get_network_summary())

@application.route('/captures/<path:path>')
def homecapture(path):
    return flask.send_from_directory('captures', path)

#if __name__ == "__main__":
#    application.run(host='0.0.0.0', port='5000')
