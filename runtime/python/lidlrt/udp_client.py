import socket
from .buffer import Memory

def udp_client(service_type, endpoint):
    fs = service_type()

    sock = socket.socket(socket.AF_INET,
                         socket.SOCK_DGRAM)

    def send_receive(x):
        sock.sendto(bytes(x), endpoint)
        print(f"Sending {x.hex()}")
        data, addr = sock.recvfrom(2048)
        print(f"Received {data.hex()}")
        return Memory(data)

    setattr(fs, "send_receive", send_receive)

    return fs

