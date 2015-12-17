#!/usr/bin/env python

__author__ = "hugh, benj.renard@gmail.com, vmon@equalit.ie"

import logging
import optparse
import re
import sys
import threading

import zmq
from zmq.eventloop import ioloop, zmqstream

ACTION_LOG = "swabber_bans"

class LogFetcher(threading.Thread):

    def __init__(self, bindstring):
        context = zmq.Context()
        self.socket = context.socket(zmq.SUB)
        subscriber = zmqstream.ZMQStream(self.socket)
        self.socket.setsockopt(zmq.SUBSCRIBE, ACTION_LOG)
        self.socket.connect(bindstring)
        threading.Thread.__init__(self)
        subscriber.on_recv(self.subscription)
        self.loop = ioloop.IOLoop.instance()

    def subscription(self, message):
        action, ipaddress = message[0:2]

        logging.debug("log is: %s %s", action, message)

    def stop(self):
        self.loop.stop()

    def run(self):
        self.loop.start()

def main():
    parser = optparse.OptionParser()

    parser.add_option("-B", "--bindstring",
                      action="store", dest="bindstring",
                      default="tcp://127.0.0.1:22620",
                      help="URI to bind to")

    parser.add_option("-L", "--logfile",
                      action="store", dest="logfile",
                      default="/usr/local/trafficserver/logs/logfetcher.log",
                      help="File to log to")

    (options, args) = parser.parse_args()

    mainlogger = logging.getLogger()
    logging.basicConfig(level=logging.DEBUG)
    log_stream = logging.StreamHandler(sys.stdout)
    log_stream.setLevel(logging.DEBUG)
    formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    log_stream.setFormatter(formatter)
    mainlogger.addHandler(log_stream)

    lfetcher = LogFetcher(options.bindstring)
    lfetcher.run()

if __name__ == "__main__":
    main()
