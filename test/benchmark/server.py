from http.server import BaseHTTPRequestHandler, HTTPServer
import sys

host_name = "localhost"
server_port = int(sys.argv[1])
server_name = sys.argv[2]


class MyServer(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        self.wfile.write(bytes(f"Hello World: {server_name} \n", "utf-8"))


if __name__ == "__main__":
    webServer = HTTPServer((host_name, server_port), MyServer)
    try:
        webServer.serve_forever()
    except KeyboardInterrupt:
        pass
    webServer.server_close()
