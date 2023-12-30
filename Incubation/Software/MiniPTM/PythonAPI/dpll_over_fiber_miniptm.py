import time
from collections import deque
from board_miniptm import *
from renesas_cm_registers import *


# Higher level board wide state machine for using DPLL over fiber
# for time and frequency transfer
class DPOF_State(Enum):
    DETECT = 1  # try to figure out which channels are PWM enabled
    ENUMERATE = 2  # of the ones that are enabled, step through and get info
    ENUMERATE_FAST = 3  # subset of ENUMERATE, when talking over PWM
    # found a partner and engaging them in fast PWM data transfer
    LOCK = 4  # found a partner who is superior, follow them
    LOCK_EXPLORE = 5  # still have a partner I'm following, but check for others
    LOCK_EXPLORE_FAST = 6  # similar idea to enumerate, checking for others
    # found a partner and engaging in fast PWM data transfer


#######################
# CHATGPT CODE , for more or less operating DPLL as a "NIC"


# Base layer representing a single PWM TX Channel

class SenderStateMachine:
    def __init__(self, board, tod_num=0):
        self.state = "Idle"
        self.tod_num = 0
        self.packet_queue = []
        self.sequence_number = 0
        self.MAX_PAYLOAD_SIZE = 4  # Maximum payload size in bytes
        self.board = board

        # enable TOD
        self.board.dpll.modules["TOD"].write_field(self.tod_num,
                                                   "TOD_CFG", "TOD_ENABLE", 1)

        # enable TOD encoder
        self.board.dpll.modules["PWMEncoder"].write_field(self.tod_num,
                                                          "PWM_ENCODER_CMD", "TOD_AUTO_UPDATE", 1)
        self.board.dpll.modules["PWMEncoder"].write_field(self.tod_num,
                                                          "PWM_ENCODER_CMD", "TOD_TX", 1)
        self.board.dpll.modules["PWMEncoder"].write_field(self.tod_num,
                                                          "PWM_ENCODER_CMD", "ENABLE", 1)

    def handle_data(self, data):
        """ Entry point to handle new data to send. """
        if self.state == "Idle":
            self._fragment_and_queue_data(data)
            self.state = "Buffering"

    def _fragment_and_queue_data(self, data):
        """ Breaks down the data into packets and adds them to the queue. """
        while data:
            fragment = data[:self.MAX_PAYLOAD_SIZE]
            data = data[self.MAX_PAYLOAD_SIZE:]
            packet = self._create_packet(fragment, continuation=bool(data))
            self.packet_queue.append(packet)

    def _create_packet(self, data, continuation):
        """ Create a data packet with header and payload, applying bit masks to fields. """
        payload_size = len(data)
        continuation_flag = 0x08 if continuation else 0x00

        # Apply bit masks to each field
        header_flag_masked = 0x80  # Fixed value, occupies 1 bit (bit 7)
        # Occupies 3 bits (bits 4-6)
        payload_size_masked = (payload_size << 4) & 0x70
        # Occupies 3 bits (bits 0-2)
        sequence_number_masked = self.sequence_number & 0x07
        # Occupies 1 bit (bit 3), double for ack
        continuation_flag_masked = (continuation_flag & 0x01) << 3

        # Combine the fields to form the header
        header = header_flag_masked | payload_size_masked | sequence_number_masked | continuation_flag_masked

        self.sequence_number = (self.sequence_number + 1) % 7

        # Packet format: [Header] + [Data Payload] + [Reserved/TOD Ticks]
        packet = [header] + data + [0] * \
            (self.MAX_PAYLOAD_SIZE - len(data)) + [0x00]
        # print(f" Create packet {packet}")
        return packet

    def _set_tod_counter(self, packet):
        """ Set the TOD counter for the specified port with the packet. """
        hex_val = [hex(val) for val in packet]
        print(
            f"Setting TOD counter for board {self.board.board_num} port {self.tod_num} with packet: {hex_val}, {packet}")
        tod_sec = 0
        for i in range(len(packet)):
            # reverse order packet, header is first
            tod_sec += packet[i] << (8*(5-i))

            # set nanoseconds to a large value, let it roll over quick
            # here basically 1usecond off from second
            self.board.write_tod_absolute(self.tod_num, 0, 999999000, tod_sec)

            self.last_tod = tod_sec  # keep track of this

    def tick(self):
        """ Simulates the passing of time or external events triggering state changes. """
        print(f"Board {self.board.board_num} sender state machine tick")
        if self.state == "Buffering" and self.packet_queue:
            packet = self.packet_queue.pop(0)
            self._set_tod_counter(packet)
            self.state = "Transmitting"
        elif self.state == "Transmitting":
            if self._has_tod_rolled_over():
                self.state = "Waiting"
        elif self.state == "Waiting":
            if not self.packet_queue:
                self.state = "Idle"
            else:
                self.state = "Buffering"

    def _has_tod_rolled_over(self):
        """ Check if the TOD has rolled over for the specified port. """
        #  read TOD immediately
        self.board.dpll.modules["TODReadPrimary"].write_reg(self.tod_num,
                                                            "TOD_READ_PRIMARY_CMD", 0x1)  # single shot, immediate

        cur_tod = self.board.dpll.modules["TODReadPrimary"].read_reg_mul(self.tod_num,
                                                                         "TOD_READ_PRIMARY_SECONDS_0_7", 6)
        cur_tod_val = 0
        for i in range(len(cur_tod)):
            cur_tod_val += cur_tod[i] << (8*i)
        hex_val = [hex(val) for val in cur_tod]
        print(f"Board {self.board.board_num} Read cur_tod {hex_val} <-> {cur_tod} seconds {cur_tod_val} , sent tod {self.last_tod}")

        if (cur_tod_val > self.last_tod):
            print(f"TOD Rolled over!")
            return True
        else:
            print(f"TOD did not rollover!")
            return False

    def get_queue_length(self):
        """ Returns the number of packets remaining in the queue. """
        return len(self.packet_queue)

    def flush_and_reset(self):
        """ Empties the queue and resets the state machine to its initial state. """
        self.packet_queue.clear()
        self.state = "Idle"
        self.current_port = 0
        self.sequence_number = 0

    # Additional methods can be added as needed


# Top level manager for all PWM TX

class DeviceSenderStateMachine:
    def __init__(self, board):
        self.transmitters = [SenderStateMachine(board, i) for i in range(4)]
        self.board = board

    def handle_data(self, port, data):
        """ Handle data for a specific transmitter. """
        if 0 <= port < len(self.transmitters):
            self.transmitters[port].handle_data(data)
        else:
            raise ValueError("Invalid port number")

    def tick(self):
        """ Update the state of each transmitter. """
        print(f"Board {self.board.board_num} DeviceSenderStateMachine tick")
        for transmitter in self.transmitters:
            transmitter.tick()

    def get_queue_length(self, port):
        """ Get the number of packets remaining in the queue for a specific transmitter. """
        if 0 <= port < len(self.transmitters):
            return self.transmitters[port].get_queue_length()
        else:
            raise ValueError("Invalid port number")

    def flush_and_reset(self, port):
        """ Flush and reset a specific transmitter. """
        if 0 <= port < len(self.transmitters):
            self.transmitters[port].flush_and_reset()
        else:
            raise ValueError("Invalid port number")

    def flush_and_reset_all(self):
        """ Flush and reset all transmitters. """
        for transmitter in self.transmitters:
            transmitter.flush_and_reset()


# Top level transmit
class DataExchangeProtocol:
    # Message Type Indicators
    QUERY_STRING = 0x01
    QUERY_NUMERIC = 0x02
    SET_NUMERIC = 0x03

    def __init__(self, device_state_machine):
        self.device = device_state_machine

    def query_string(self, port, string_id):
        """ Query for a specific string of data. """
        payload = [self.QUERY_STRING, string_id]
        self.device.handle_data(port, payload)

    def query_numeric(self, port, numeric_id):
        """ Query for numerical data of a known identifier. """
        payload = [self.QUERY_NUMERIC, numeric_id]
        self.device.handle_data(port, payload)

    def set_numeric_value(self, port, address, value):
        """ Set a specific numerical value at a given address. """
        payload = [self.SET_NUMERIC, address] + self._value_to_bytes(value)
        self.device.handle_data(port, payload)

    def _value_to_bytes(self, value):
        """ Convert a numerical value to a list of bytes. """
        # Assuming value is an integer, adjust as needed for other types
        # Adjust size and byte order as needed
        return value.to_bytes(4, byteorder='big')


# Receive stack all in one
class ReceiverProtocol:
    def __init__(self, board):
        self.partial_message = b''
        self.last_packet = None
        self.buffer_change_count = 0
        self.got_query = []
        self.MAX_PAYLOAD_SIZE = 4  # Maximum payload size in bytes
        self.board = board
        # disable all decoders
        for i in range(len(self.board.dpll.modules["PWMDecoder"].BASE_ADDRESSES)):
            self.board.dpll.modules["PWMDecoder"].write_field(i,
                                                              "PWM_DECODER_CMD", "ENABLE", 0)

    def tick(self):
        print(f"Board {self.board.board_num} ReceiverProtocol tick")
        got_ack, ack_packet_to_send = self.process_incoming_data()
        return got_ack, ack_packet_to_send

    def enable_port(self, port):
        """ Enable a specific port for receiving data. """
        print(f"PWM Receiver Board {self.board.board_num} enable port {port}")
        if 0 <= port <= 3:
            # SFP ports are off by 1, ports 0 / 2 / 4 / 6
            self.board.dpll.modules["PWMDecoder"].write_field(port*2,
                                                              "PWM_DECODER_CMD", "ENABLE", 1)

        else:
            raise ValueError("Invalid port number")

    def disable_port(self, port):
        """ Disable a specific port to stop receiving data. """
        print(f"PWM Receiver Board {self.board.board_num} disable port {port}")
        if 0 <= port <= 3:
            # SFP ports are off by 1, ports 0 / 2 / 4 / 6
            self.board.dpll.modules["PWMDecoder"].write_field(port*2,
                                                              "PWM_DECODER_CMD", "ENABLE", 0)
        else:
            raise ValueError("Invalid port number")

    def read_hardware_buffer(self):
        """ Read data from the hardware's global receive buffer. """
        # just read seconds portion
        data = self.board.i2c.read_dpll_reg_multiple(0xce80, 0x5, 6)
        hex_val = [hex(val) for val in data]
        print(f"Read PWM Receive Board {self.board.board_num} TOD {hex_val}")
        return data

    def _is_data_packet(self, data):
        """ Check if the received data is a data packet. """
        return data[5] & 0x80 == 0x80

    # returns if it received it ACK packet
    # and the ack packet to send if needed
    def process_incoming_data(self):
        """ Process incoming data from the hardware buffer. """
        packet = self.read_hardware_buffer()
        if packet and self._is_data_packet(packet) and packet != self.last_packet:
            self.last_packet = packet
            self.buffer_change_count += 1
            was_ack = self.process_packet(packet)
            return was_ack, self.get_acknowledge_packet(packet)
        return False, []

    def get_acknowledge_packet(self, in_packet):
        """ Create a data packet with header and payload, applying bit masks to fields. """
        seq_num = in_packet[0] & 0x7
        payload_size = 0

        # Apply bit masks to each field
        header_flag_masked = 0x80  # Fixed value, occupies 1 bit (bit 7)
        # Occupies 3 bits (bits 4-6)
        payload_size_masked = (payload_size << 4) & 0x70
        sequence_number_masked = seq_num & 0x07  # Occupies 3 bits (bits 0-2)
        # Occupies 1 bit (bit 3), doubles for ack
        continuation_flag_masked = 1 << 3

        # Combine the fields to form the header
        header = header_flag_masked | payload_size_masked | sequence_number_masked | continuation_flag_masked

        data = []  # nothing
        # Packet format: [Header] + [Data Payload] + [Reserved/TOD Ticks]
        packet = [header] + data + [0] * \
            (self.MAX_PAYLOAD_SIZE - len(data)) + [0x00]
        return packet

    def process_packet(self, packet):
        """ Process an individual packet. """
        if not packet:
            return False
        # it's in reverse order from how it's transmitted, flip it to make it more sequential
        packet = packet.reverse()

        header = packet[0]
        payload = packet[1:]
        if header & 0x80 == 0x80:  # Check if it's a data packet
            payload_size = (header & 0x70) >> 4
            continuation = header & 0x08 == 0x08
            message_type = payload[0]
            message_data = payload[1:1+payload_size]

            if (payload_size == 0 and continuation):  # ack
                print(f"Received ack packet")
                self.partial_message = b''
                return True

            # Handling long messages
            self.partial_message += bytes(message_data)
            if not continuation:
                self.handle_complete_message(
                    message_type, self.partial_message)
                self.partial_message = b''
        return False

    def handle_complete_message(self, message_type, message):
        """ Handle a complete message based on its type. """
        self.got_query = [message_type, message]
        print(f"Got complete message {message_type} -> {message}")
        return
        if message_type == DataExchangeProtocol.QUERY_STRING:
            # Process string query
            string_id = message[0]
            self.handle_string_query(string_id)
        elif message_type == DataExchangeProtocol.QUERY_NUMERIC:
            # Process numeric query
            numeric_id = message[0]
            self.handle_numeric_query(numeric_id)
        elif message_type == DataExchangeProtocol.SET_NUMERIC:
            # Process set numeric value
            address, value = message[0], int.from_bytes(
                message[1:], byteorder='big')
            self.handle_set_numeric(address, value)

    def handle_string_query(self, string_id):
        """ Handle a string query request. """
        # Logic to handle string query
        pass

    def handle_numeric_query(self, numeric_id):
        """ Handle a numeric query request. """
        # Logic to handle numeric query
        pass

    def handle_set_numeric(self, address, value):
        """ Handle setting a numeric value. """
        # Logic to set the numeric value
        pass


class DPOF_Top():
    def __init__(self, board):
        print(f"DPOF_Top init board {board.board_num}")
        self.RX = ReceiverProtocol(board)
        self.TX = DeviceSenderStateMachine(board)
        self.TX_Protocol = DataExchangeProtocol(self.TX)
        self.board = board
        self.state = DPOF_State.DETECT
        self.cur_rx_port = -1
        self.cur_rx_port_pktcnt = 0
        self.start_time = -1
        print(f"DPOF_Top done init board {board.board_num}")

    def run_loop(self):
        print(f"HI FROM DPOF")
        if ( self.state == DPOF_State.DETECT):
            print(f" IN DETECT STATE")
            self.detect_state()
        print(f" Done with state processing")
        self.TX.tick()
        got_ack, ack_packet_to_send = self.RX.tick()
        hex_val = [hex(val) for val in ack_packet_to_send]
        print(f"RX Tick, got_ack={got_ack}, ack_to_send={hex_val} <-> {ack_packet_to_send}")
        if (len(self.RX.got_query)):
            print(f"Got full query from other side!")


    def detect_state(self):
        print(f"DPOF Top board {self.board.board_num} detect state function")
        if (self.cur_rx_port == -1):  # first time
            self.RX.enable_port(0)
            self.cur_rx_port = 0
            self.cur_rx_port_pktcnt = 0
            self.start_time = time.time()
            # send out a query by default on all ports
            for port in range(4):
                self.TX_Protocol.query_numeric(port, 0xa)

        if (time.time() - self.start_time > 5 and
                self.cur_rx_port_pktcnt == 0):
            # been on this port for 5 seconds
            # haven't received any packets
            # move to next port
            self.RX.disable_port(self.cur_rx_port)
            self.cur_rx_port = (self.cur_rx_port + 1) % 4
            self.cur_rx_port_pktcnt = 0
            self.RX.enable_port(self.cur_rx_port)
