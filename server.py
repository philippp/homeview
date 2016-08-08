#!/usr/bin/env python
import os
import os.path
import math
import flask
import pprint
import xmltodict
import pdb
import collections
import datetime
import csv
import opencv.transition_features_pb2 as transition_features

application = flask.Flask(__name__)

def get_network_summary(datestamp):
    fd = open("captures/%s/network.xml" % datestamp,'r')
    doc = xmltodict.parse(fd.read())
    if not 'nmaprun' in doc.keys() or not 'host' in doc['nmaprun'].keys():
        return "NO NMAP RUN FOUND."
    
    machines = list()
    for host in doc['nmaprun']['host']:
        machine = dict()
        if 'address' not in host:
            continue
        if isinstance(host['address'], collections.OrderedDict):
            singlenode = host['address']
            host['address'] = list()
            host['address'].append(singlenode)
        for address in host['address']:            
            if address.get('@addrtype') in ['ipv4', 'ipv6']:
                machine['ip'] = address.get('@addr')
            if address.get('@addrtype') in ['mac']:
                machine['mac'] = address.get('@addr')
                machine['vendor'] = address.get('@vendor')
        if host.get('hostnames'):
            hostnames = host['hostnames']
            if 'hostname' in hostnames and '@name' in hostnames['hostname']:
                machine['hostname'] = hostnames['hostname']['@name']
        machines.append(machine)
    return machines

def get_last_24h_datestamps():
    today_date = datetime.date.today().strftime('%Y/%m/%d')
    datestamps_today = os.listdir("captures/%s" % today_date)
    datestamps_today.sort()
    
    yesterday_date = (datetime.datetime.now() - datetime.timedelta(days=1)).strftime('%Y/%m/%d')
    datestamps_yesterday = os.listdir("captures/%s" % yesterday_date)
    datestamps_yesterday.sort()
    for i in range(len(datestamps_yesterday))[::-1]:
        if datestamps_yesterday[i] < datestamps_today[len(datestamps_today)-1]:
            break
    expand_ds = lambda root, ds_list: ["%s/%s" % (root, ds) for ds in ds_list]
    datestamps = (expand_ds(yesterday_date, datestamps_yesterday[i:]) +
                  expand_ds(today_date, datestamps_today))                  
    return datestamps


@application.route("/")
def hello():
    datestamp = flask.request.args.get('datestamp', 'latest')
    listed_datestamps = get_last_24h_datestamps()
    return flask.render_template('index.html',
                                 machines=get_network_summary(datestamp),
                                 datestamp=datestamp,
                                 listed_datestamps=listed_datestamps)


@application.route("/analysis/<year>/<month>/<day>", defaults={'prefix_filter': ""})
@application.route("/analysis/<year>/<month>/<day>/<prefix_filter>")
def analysis(year, month, day, prefix_filter):
    filename = "captures/%s/%s/%s/analysis" % (year, month, day)
    if len(prefix_filter):
        filename += "_%s" % (prefix_filter)
    filename += "/sequence_metadata.rio"
    if not os.path.isfile(filename):
        return "File not found"

    sequence = transition_features.TransitionSequence()    
    sequence.ParseFromString(open(filename, 'rb').read())
    for t in sequence.transitions:
        t.score = int(t.rss_distance_variance * math.sqrt(float(t.interest_box.rss_distance_mean)/t.rss_distance_mean))
    ranked_sequence = sorted(sequence.transitions,
                             key=lambda d: d.score)[::-1]
    ranked_sequence = ranked_sequence[:100]
    return flask.render_template('analysis.html',
                                 ranked_sequence=ranked_sequence)

# When running locally (without nginx), serve files locally.
local_captures_path = '/disabled_non_local'
if __name__ == "__main__":
    local_captures_path = '/captures/<path:path>'
    
@application.route(local_captures_path)
def send_captures(path):
    if __name__ == "__main__":
        return flask.send_from_directory('captures', path)
    else:
        return "disabled_non_local"


if __name__ == "__main__":
    application.config['DEBUG'] = True
    application.run(host="0.0.0.0", port=8000)
