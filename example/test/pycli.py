#!/usr/bin/env python

import cmd
import sys
from com import Com

#test based on http://en.wikipedia.org/wiki/JSON-RPC
#and follow the specification on http://www.jsonrpc.org/specification

class Cmd_line(cmd.Cmd):
    def __init__(self, completekey='tab', stdin=None, stdout=None):
        cmd.Cmd.__init__(self, completekey=completekey, stdin=stdin, stdout=stdout)
        self.com = Com()
        self.do_connect("")

    def do_EOF(self, line):
        # Redo last command
        return True

    def do_connect(self, line):
        self.com.connect(add = "localhost", port = 1234)

    ########################################################################
    # List of command                                                      #
    ########################################################################
    def do_helloworld(self, line):
        print(self.com.send("sayHello"))

    def do_helloworld_param(self, line):
        print(self.com.send("sayHello", param=line))

    def do_exit(self, line):
        print(self.com.send("exit"))
        sys.exit(0)

    def do_test_not_exist_procedure(self, line):
        print(self.com.send("never_exit"))

    def do_test_invalid_json(self, line):
        json = '{"jsonrpc": "2.0", "method": "foobar", "params": "bar", "baz"]'
        self.com.send(None, force_json=json)

    def do_test_invalid_json_rpc(self, line):
        print "test 1"
        json = '[1, 2,0]'
        self.com.send(None, force_json=json)
        print "test 2"
        json = '{"jsonrpc": "2.0", "method": "sayHello", "params": "bar", "id": 1}'
        self.com.send(None, force_json=json)

    def do_test_wrong_json_version(self, line):
        json = '{"jsonrpc": "2.1", "method": "sayHello"}'
        self.com.send(None, force_json=json)

    def do_test_send_null_id(self, line):
        json = '{"jsonrpc": "2.0", "method": "sayHello", "id": null}'
        self.com.send(None, force_json=json)

    def do_test_send_fractional_id(self, line):
        json = '{"jsonrpc": "2.0", "method": "sayHello", "id": 1.5}'
        self.com.send(None, force_json=json)

    def do_helloworld_notify(self, line):
        self.com.send_notify("sayHello")

if __name__ == '__main__':
    Cmd_line().cmdloop()

