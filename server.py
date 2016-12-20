#!/usr/bin/env python

"""Serve watkins antichess proof trees."""

import chess.variant

import aiohttp.web

import sys
import logging
import asyncio
import argparse
import os.path
import json


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
        logging.debug("%d << %s", self.transport.get_pid(), line)
        self.transport.get_pipe_transport(0).write((line + "\n").encode("utf-8"))

    def line_received(self, line):
        logging.debug("%d >> %s", self.transport.get_pid(), line)
        if line.startswith("prolog "):
            _, self.prolog = line.split(None, 1)
        elif line == "Ready for input":
            logging.info("Subprocess %d is ready for input, prolog: %s", self.transport.get_pid(), self.prolog or "<none>")
            self.ready.set()
        elif line.startswith("Val:") or line == "Not in BOOK" or line == "No children":
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
            self.send_line("query " + query)
            yield from self.query_ready.wait()

            return self.query_result
        finally:
            self.query_result = None
            self.semaphore.release()


def jsonp(request, obj):
    json_str = json.dumps(obj, indent=2, sort_keys=True)

    callback = request.GET.get("callback")
    if callback:
        return aiohttp.web.Response(
            text="%s(%s)" % (callback, json_str),
            content_type="application/javascript")
    else:
        return aiohttp.web.Response(
            text=json_str,
            content_type="application/json",
            headers={
                "Access-Control-Allow-Origin": "*"
            })


class Api:

    def __init__(self, proofs):
        self.drivers = []

        loop = asyncio.get_event_loop()

        for proof in proofs:
            _, driver = loop.run_until_complete(
                loop.subprocess_exec(
                    Protocol,
                    os.path.join(os.path.dirname(__file__), "LOSINGv1", "parse"),
                    proof))

            self.drivers.append(driver)

    @asyncio.coroutine
    def query(self, request):
        board = chess.variant.GiveawayBoard()
        moves = []

        try:
            post = yield from request.post()
            for uci in post.get("moves", request.GET.get("moves", "")).split():
                board.push_uci(uci)
                moves.append(uci)
        except ValueError:
            raise aiohttp.web.HTTPBadRequest()

        q = " ".join(moves)

        response = {
            "variant_win": board.is_variant_win(),
            "variant_loss": board.is_variant_loss(),
            "moves": []
        }

        for driver in self.drivers:
            if q.startswith(driver.prolog):
                result = yield from driver.query(q)

                for move in result:
                    move["san"] = board.san(board.parse_uci(move["uci"]))

                if result:
                    response["moves"] = result
                    break

        return jsonp(request, response)


def main(args):
    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    api = Api(args.proofs)

    app = aiohttp.web.Application()
    app.router.add_route("POST", "/", api.query)
    app.router.add_route("GET", "/", api.query)

    aiohttp.web.run_app(app, host=args.host, port=args.port)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-v", "--verbose", action="store_true", help="log debug messages")
    parser.add_argument("--host", default="127.0.0.1", help="bind interface")
    parser.add_argument("-p", "--port", type=int, default=5005, help="http port (default: 5005)")
    parser.add_argument("proofs", nargs="+")
    main(parser.parse_args())
