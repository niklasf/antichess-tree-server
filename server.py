#!/usr/bin/env python

import sys
import logging
import asyncio


class Protocol(asyncio.SubprocessProtocol):

    def __init__(self):
        super(Protocol, self).__init__()
        self.prolog = ""

        self.ready = asyncio.Event()
        self.query_ready = asyncio.Event()
        self.semaphore = asyncio.Semaphore()

        self.buf = bytearray()
        self.transport = None

        self.query_result = None

    def connection_made(self, transport):
        super(Protocol, self).connection_made(transport)
        self.transport = transport

    def connection_lost(self, exc):
        super(Protocol, self).connection_lost(exc)
        logging.error("Subprocess %d died, exception: %s", self.transport.get_pid(), exc or "<none>")
        self.transport = None
        self.ready.clear()

    def pipe_data_received(self, fd, data):
        self.buf.extend(data)
        while ord("\n") in self.buf:
            prefix, self.buf = self.buf.split(b"\n", 1)
            self.line_received(prefix.decode("utf-8"))

    def send_line(self, line):
        self.transport.get_pipe_transport(0).write((line + "\n").encode("utf-8"))

    def line_received(self, line):
        logging.debug("%d >> %s", self.transport.get_pid(), line)
        if line.startswith("prolog "):
            _, self.prolog = line.split(None, 1)
        elif line == "Ready for input":
            logging.info("Subprocess %d is ready for input, prolog: %s", self.transport.get_pid(), self.prolog or "<none>")
            self.ready.set()
        elif line.startswith("Val:") or line == "Not in BOOK":
            pass
        elif line == "QUERY COMPLETE":
            self.query_ready.set()
        elif self.query_result is not None:
            uci, nodes, _, _ = line.split(None, 3)
            self.query_result.append({
                "uci": uci,
                "nodes": int(nodes)
            })

    @asyncio.coroutine
    def query(self, query):
        yield from self.ready.wait()

        try:
            yield from self.semaphore.acquire()

            self.query_result = []

            self.query_ready.clear()
            self.send_line("query e2e3")
            yield from self.query_ready.wait()

            return self.query_result
        finally:
            self.query_result = None
            self.semaphore.release()


class Driver:
    def __init__(self, loop, proof):
        self.transport, self.protocol = loop.run_until_complete(loop.subprocess_exec(Protocol, "./LOSINGv1/parse", "easy12.done"))


if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG)
    loop = asyncio.get_event_loop()
    d = Driver(loop, sys.argv[1])
    print(loop.run_until_complete(d.protocol.query("foo")))
    loop.run_forever()
    loop.close()
