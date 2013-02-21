#!/usr/bin/env python

import socket
import json
import sys

class Com():
    def __init__(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.next_id = 1

    def connect(self, add = "localhost", port = 1234):
        self.socket.connect((add, port))

    def send(self, method, param=None, force_json=None, notify=False):
        """
            Description : 
                Call rpc function by socket
            params : 
                - method : string, the rpc method name
                - param : string, the param to pass to the method function
            return :
                - return the answer of the rpc method
        """
        if self.socket:
            if force_json:
                self.socket.send(force_json)
            else:
                self.socket.send(self.__request(method, param, notify=notify))
            if not notify:
                answer = self.socket.recv(1024).strip()
                if not answer:
                    answer = self.socket.recv(1024).strip()
                print answer
                if force_json:
                    return answer
                else:
                    return self.__result(answer)
            else:
                return
        raise ValueError("Socket is close.");

    def send_notify(self, method, param=None, force_json=None):
        self.send(method, param=param, force_json=force_json, notify=True)

    def close(self):
        self.socket.close()
        self.socket = None

    def __request(self, method, param, notify=False):
        sParam = ', "params": ["%s"]' % param if param else ""
        if notify:
            return '{"jsonrpc": "2.0", "method": "%s"%s}' % (method, sParam)
        sId = "%s" % self.next_id
        if self.next_id == sys.maxint:
            self.next_id = 0
        self.next_id += 1
        return '{"jsonrpc": "2.0", "method": "%s"%s, "id": %s}' % (method, sParam, sId)

    def __result(self, result):
         return json.loads(result).get("result", None)

