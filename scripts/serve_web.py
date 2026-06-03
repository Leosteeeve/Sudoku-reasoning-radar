from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import argparse
import os


class NoCacheHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0")
        self.send_header("Pragma", "no-cache")
        self.send_header("Expires", "0")
        super().end_headers()


def main():
    parser = argparse.ArgumentParser(description="Serve docs/play with no browser cache.")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--directory", default=os.path.join("docs", "play"))
    args = parser.parse_args()

    handler = partial(NoCacheHandler, directory=args.directory)
    server = ThreadingHTTPServer(("localhost", args.port), handler)
    print(f"Serving {args.directory} at http://localhost:{args.port}/")
    print("Cache disabled. Press Ctrl+C to stop.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping server.")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
