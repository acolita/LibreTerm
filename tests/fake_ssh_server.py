import socket
import threading
import time
import sys

def handle_client(client_socket):
    try:
        # Send SSH version string to satisfy PuTTY's initial handshake expectation
        client_socket.sendall(b"SSH-2.0-OpenSSH_8.2p1 Ubuntu-4ubuntu0.1\r\n")
        
        # Read client version
        client_socket.recv(1024)
        
        # Just keep the connection open to simulate a "connected" state
        # PuTTY will eventually timeout if handshake doesn't complete, 
        # but it should be enough for UI tests.
        while True:
            time.sleep(1)
            # Send keepalive or just empty data? 
            # sending empty might break pipe. Just sleeping is usually enough for a minute.
    except Exception as e:
        print(f"Client error: {e}")
    finally:
        client_socket.close()

def start_server(port=2222):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        server.bind(('127.0.0.1', port))
        server.listen(5)
        print(f"Fake SSH Server listening on port {port}...")
        sys.stdout.flush()
        
        while True:
            client, addr = server.accept()
            t = threading.Thread(target=handle_client, args=(client,))
            t.daemon = True
            t.start()
    except Exception as e:
        print(f"Server error: {e}")
    finally:
        server.close()

if __name__ == "__main__":
    start_server()
