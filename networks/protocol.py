import socket
import struct
import random

class UDPBasedProtocol:
    def __init__(self, *, local_addr, remote_addr):
        self.udp_socket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
        self.remote_addr = remote_addr
        self.udp_socket.bind(local_addr)

    def sendto(self, data):
        return self.udp_socket.sendto(data, self.remote_addr)

    def recvfrom(self, n):
        msg, addr = self.udp_socket.recvfrom(n)
        return msg

    def close(self):
        self.udp_socket.close()

RETRIES = 5

class TCPPacket:
    HEADER_FORMAT = "BIQ"
    HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
    MAX_PACKET_SIZE = 4096
    DATA_SIZE = MAX_PACKET_SIZE - HEADER_SIZE

    TYPE_DATA = 0x00
    TYPE_ACK = 0x01
    TYPE_FIN = 0x02

    def __init__(self, type: int, seq: int, message_id: int = None, data: bytes = None):
        self.type = type
        self.data = data
        self.seq = seq
        if message_id is not None:
            self.message_id = message_id
        else:
            self.random_id()

    def random_id(self):
        self.message_id = random.getrandbits(64)

    def __bytes__(self):
        header = struct.pack(TCPPacket.HEADER_FORMAT, self.type, self.seq, self.message_id)
        return header + self.data if self.data else header
    
    @classmethod
    def from_bytes(_, raw_packet: bytes):
        type, seq, message_id = struct.unpack(TCPPacket.HEADER_FORMAT, raw_packet[:TCPPacket.HEADER_SIZE])
        data = raw_packet[TCPPacket.HEADER_SIZE:]
        return TCPPacket(type, seq, message_id, data)


class MyTCPProtocol(UDPBasedProtocol):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.udp_socket.settimeout(0.005)
        self.sender_buffer = set()
        self.receiver_buffer = set()

    def __data_to_packets(self, data: bytes) -> list[TCPPacket]:
        packets, seq = [], 0
        chunks = [data[i:i+TCPPacket.DATA_SIZE]
                  for i in range(0, len(data), TCPPacket.DATA_SIZE)]
        for i, chunk in enumerate(chunks):
            type = TCPPacket.TYPE_DATA if i + 1 < len(chunks) else TCPPacket.TYPE_FIN
            packets.append(TCPPacket(type=type, seq=seq, data=chunk))
            seq += len(chunk)
        return packets

    def __send_packet(self, packet: TCPPacket, current_sequence: int):
        try:
            packet.random_id()
            self.sendto(bytes(packet))
            while True:
                response = self.recvfrom(TCPPacket.MAX_PACKET_SIZE)
                ack_packet = TCPPacket.from_bytes(response)
                if ack_packet.message_id in self.sender_buffer:
                    continue
                break
            self.sender_buffer.add(ack_packet.message_id)
            if ack_packet.type == TCPPacket.TYPE_ACK and ack_packet.seq == current_sequence:
                return True
        except TimeoutError:
            pass

        return False

    def send(self, data: bytes):
        seq = 0
        for packet in self.__data_to_packets(data):
            seq += len(packet.data)
            received = False
            while not received:
                received = self.__send_packet(packet, seq)
        return len(data)

    def __receive_packet(self):
        while True:
            try:
                response = self.recvfrom(TCPPacket.MAX_PACKET_SIZE)
                packet = TCPPacket.from_bytes(response)
                if packet.message_id in self.receiver_buffer:
                    continue
                self.receiver_buffer.add(packet.message_id)
                if packet.type == TCPPacket.TYPE_ACK:
                    continue
                return packet
            except:
                continue

    def recv(self, n: int):
        data = bytes()
        while len(data) < n:
            packet = self.__receive_packet()
            if packet.seq == len(data):
                data += (packet.data)
                ack_packet = TCPPacket(type=TCPPacket.TYPE_ACK, message_id=packet.message_id, seq=len(data))
                self.sendto(bytes(ack_packet))
                if packet.type == TCPPacket.TYPE_FIN:
                    for _ in range(RETRIES):
                        ack_packet.random_id()
                        self.sendto(bytes(ack_packet))
                    return data
            else:
                ack_packet = TCPPacket(type=TCPPacket.TYPE_ACK, message_id=packet.message_id, seq=packet.seq + len(packet.data))
                self.sendto(bytes(ack_packet))

    def close(self):
        super().close()
        