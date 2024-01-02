import time
from collections import deque
from board_miniptm import *
from renesas_cm_registers import *
import random



"""
For DPLL over fiber, it relies on three mechanisms:
    1. TOD transmission by the encoders
        a. Each encoder has it's own TOD
        b. effectively 4 TX buffers
    2. TOD reception by the decoders
        a. Only one RX buffer, PWM_TOD 0xce80
        b. only seconds portion is used, but 6 bytes of second data
    3. PWM User Data transfer
        a. Only one buffer for TX and RX
        b. 128 byte 


For anything more than a few bytes, PWM User Data is the only reasonable mechanism

Receive path is most constrained
    1. Only one decoder should be enabled at a time
    2. It may take time for a new frame to come in, even if other side wants to transmit

Design mainly around optimizing the receive Path

Goal of communication with TOD transmission / reception is to negotiate User data transfer

Define protocol using upper two bytes of TOD, TOD_SEC[5] and TOD_SEC[4]

Assume these second fields are dedicated, only using 4 bytes of seconds, 132 years good enough

Protocol is a handshake process
    1. One or both sides sends an initial (0x0) state data TOD with desired action and a random byte

    2. If both random numbers are the same, then both sides must change their number and wait for other side to change too

    3. Whichever side has the lowest random byte wins priority for it's action to be completed

    4. If the winning side wants to query something, then it readies it's PWM user data for reception, otherwise it readies it for transmission

    5. LOSING SIDE
        a. If the request is a query, the losing side readies its PWM User data for transmission, but doesnt sent it yet, it sends accept (0x1) state data TOD
        b. If the request is a write, the losing side readies its PWM User data for reception (clearing first byte of buffer), and sends accept (0x1) state data TOD

    6. The winning side, upon reception of accept, also sends accept (0x1) state data 
        a. If the request is a query, it waits for PWM User data reception completion 
        b. If the request is a write, it goes through process of sending user data

    7. If the request is a query or a write, the losing side sends end state (0x2) TOD first, and uses random byte field to send its PWM User status
        a. If the request is a write, losing side RX side stores this full buffer and passes to higher layer, 128 byte buffer format defined elsewhere
        b. If the request is a query, winner side RX side stores this full buffer and passes to higher layer, 128 byte buffer format defined elsewhere

    8. Two options for winner
        a. If done with transmit , go back to normal TOD transmission without data flag set
        b. If more data to transmit, send initial (0x0) state data TOD with desired action. Repeated start kind of behavior, go back to 4

    9. For receiver
        a. If see data flag go away, then transaction is completed, handle data buffer however that data buffer is defined
        b. If see data flag stay and state go back to 0x0, then go back to step 4 here

Defined in the layout in registers map but description is here PWM_RX_INFO_LAYOUT

TOD_SEC[5][7] = Data Flag, set when the TOD field is being aliased for handshaking, 0 when normal TOD
TOD_SEC[5][6:5] = Handshake flag
TOD_SEC[5][0:4] = Transaction ID
    0x0 = Read chip info
            a. Status of all inputs, STATUS.INX_MON_STATUS bytes for 0-15 (16 bytes)
            b. DPLL Status of DPLLs , STATUS.DPLLX_STATUS bytes for 0-3 (4 bytes)
            c. Input frequency monitor info, STATUS.INX_MON_FREQ_STATUS for 0-15, (32 bytes)
            d. A name string, 16 bytes including null
            e. TOD delta seen between received TOD frame and local TOD counter, used for round trip calculations (11 bytes)
            f. 

    0x1 = Write to board
            a. LED values bit-wise, 1 byte
            b. Force follow this requester, 1 byte, must be 0xa5 for this function, otherwise doesn't use
                Follow frequency and TOD and PPS

TOD_SEC[4][7:0] = Random value for winner / loser determination, PWM User data state from loser upon end state

"""




"""
RX is the most constrained , so it needs the most logic

1. Need to round robin through decoders often
2. Want to detect
    a. is there a PWM encoder on the other side
    b. is the PWM encoder on the other side trying to initiate a request
"""
class DPOF_Top():
    def __init__(self, board):
        self.board = board

        ######## RX Logic variables
        self.decoder_active = [0,0,0,0]
        self.decoder_wants_transaction = [0,0,0,0]
        self.active_decoder = 0
        self.time_on_this_decoder = time.time()
        self.round_robin_time_seconds = 5
        self.pwm_tod_before_switch = 0

        ######## TX Logic variables
        # keep track of which encoder I'm trying to start a transaction on
        self.starting_transaction = [0,0,0,0] 


        # disable all decoders
        for i in range(len(self.board.dpll.modules["PWMDecoder"].BASE_ADDRESSES)):
            self.board.dpll.modules["PWMDecoder"].write_field(i,
                  "PWM_DECODER_CMD", "ENABLE", 0)

        # clear the PWM TOD, basically store what it was when everything was disabled
        self.pwm_tod_before_switch = self.read_raw_hardware_buffer()

        self.board.dpll.modules["PWMDecoder"].write_field(self.active_decoder,
                "PWM_DECODER_CMD", "ENABLE", 1)

    # returns data from hardware buffer only if new data is present
    def read_raw_hardware_buffer(self):
        """ Read data from the hardware's global receive buffer. """
        # just read seconds portion
        data = self.board.i2c.read_dpll_reg_multiple(0xce80, 0x5, 6)
        hex_val = [hex(val) for val in data]
        print(f"Read PWM Receive Board {self.board.board_num} TOD {hex_val}")
        return data

    def go_to_next_decoder():
        self.board.dpll.modules["PWMDecoder"].write_field(self.active_decoder,
                "PWM_DECODER_CMD", "ENABLE", 0)

        # clear the PWM TOD, basically store what it was when everything was disabled
        self.pwm_tod_before_switch = self.read_raw_hardware_buffer()

        self.active_decoder = (self.active_decoder + 1) % 4



        self.board.dpll.modules["PWMDecoder"].write_field(self.active_decoder,
                "PWM_DECODER_CMD", "ENABLE", 1)
        self.time_on_this_decoder = time.time()

        if ( self.active_decoder == 0 ):
            # went through one round robin, maybe do something
            pass



    
    def handshake_complete():
        # called when I started on an encoder already
        # and I also get a response


ENDED AROUND HERE, 
    # top level TX function
    def TX_try_start_transaction(self, encoder_id=0, transaction_id=0):
        tod_sec_upper = (1<<7) + transaction_id & 0x1f
        tod_sec_lower = random.randint(0,255) 

        tod_sec = (tod_sec_upper << (8*5)) + (tod_sec_lower << (8*4))

        # do a relative TOD jump, assume upper seconds aren't rolling over
        self.board.write_tod_relative(encoder_id, 0, 0, tod_sec, True)
        self.starting_transaction[encoder_id] = 1
        

    # top level function
    def tick(self): 
        data = self.read_raw_hardware_buffer()
        if ( data == self.pwm_tod_before_switch ):
            # data hasn't changed
            if ( (time.time() - self.time_on_this_decoder) >= self.round_robin_time_seconds ):
                # given this decoder enough time, haven't received anything, go to next
                self.go_to_next_decoder()
            else:
                # data hasn't changed, but still giving this receiver time
                return []
        else:
            # received new data on this decoder, it's active at least
            self.decoder_active[self.active_decoder] = 1
            print(f"Board {self.board.board_num} found active decoder {self.active_decoder}")

            # check what the data is , does it want transaction
            if ( data[-1] & 0x80 ):
                if ( self.starting_transaction[self.active_decoder] ):
                    self.handshake_complete()
                print(f"Board {self.board.board_num} found decoder request {self.active_decoder}")
                self.decoder_wants_transaction[self.active_decoder] = 1

        return [] 



class PWM_TX():
    def __init__(self, board, tod_num=0):
        self.tod_num = tod_num
        self.board = board
        self.MAX_PAYLOAD_SIZE = 4
        self.sequence_number = 0
        self.last_tx_packet = []
        self.state = "IDLE"
        self.ack_num = 0

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



    # NOT FOR SENDING ACK PACKETS
    # payload is a list
    def write_single_payload(self, payload, is_continuation=False):
        if len(payload) > self.MAX_PAYLOAD_SIZE:
            return False
        if self.state != "IDLE":
            return False
       
        header_data = 0x80 # Data flag
        header_data += ( len(payload) & 0x7 ) << 4 # Payload size
        header_data += int(is_continuation) << 3
        header_data += (self.sequence_number & 0x7)

        self.ack_num = self.sequence_number

        self.sequence_number += 1
        self.sequence_number = self.sequence_number & 0x7

        # ok have header, now add payload together for full packet
        pkt_data = [header_data] + payload 

        tod_sec = 0
        for i in range(len(pkt_data)):
            tod_sec += pkt_data[i] << (8*(5-i))

        hex_val = [hex(val) for val in pkt_data]
        print(f"Write single payload {hex_val}")
        # write this to TOD, give it 200ms to process
        self.board.write_tod_absolute(self.tod_num, 0, 800000000, tod_sec)
        self.last_tx_packet = list(pkt_data)
        self.state = "WAIT_TX_GO_OUT_ACK"
       
    def write_ack_packet(self, header):
        header_val = header[0]
        seq_num_to_ack = header_val & 0x7

        header_data = 0x80
        header_data += 1 << 3 # continuation / ack flag
        header_data += seq_num_to_ack

        pkt_data = [header_data]

        tod_sec = 0
        for i in range(len(pkt_data)):
            tod_sec += pkt_data[i] << (8*(5-i))

        # write this to TOD, give it 200ms to process
        self.board.write_tod_absolute(self.tod_num, 0, 800000000, tod_sec)
        self.last_tx_packet = list(pkt_data)
        self.state = "WAIT_TX_GO_OUT_NO_ACK"


    def check_has_tx_gone_out(self):
        # read current TOD seconds
        #  read TOD immediately

        if ( self.is_tx_idle() ): 
            return True

        self.board.dpll.modules["TODReadPrimary"].write_reg(self.tod_num,
            "TOD_READ_PRIMARY_CMD", 0x1)  # single shot, immediate

        cur_tod_seconds = self.board.dpll.modules["TODReadPrimary"].read_reg(0,
                "TOD_READ_PRIMARY_SECONDS_0_7")
       
        # assume you're checking this at least once every 255 seconds!
        if cur_tod_seconds > 0:
            print(f"Board {self.board.board_num} TX{self.tod_num} Has gone out! Seconds rolled over {cur_tod_seconds}")
            self.last_tx_packet = []
            if ( self.state == "WAIT_TX_GO_OUT_ACK" ): 
                self.state = "WAIT_ACK"
            elif ( self.state == "WAIT_TX_GO_OUT_NO_ACK" ):
                self.state = "IDLE"
            return True
        else:
            print(f"Board {self.board.board_num} TX{self.tod_num} has not gone out, seconds = {cur_tod_seconds}")
            return False

    def got_incoming(self, header):
        data_flag = (header >> 7) & 0x1 
        ack_flag = (header >> 3 ) & 0x1
        payload_len = (header >> 4) & 0x7
        seq_num = (header & 0x7) 

        print(f"Board {self.board.board_num} TX{self.tod_num} got incoming header=0x{header:02x}")
        print(f"    Seq_num = {seq_num}, ack_num = {self.ack_num}, data={data_flag}, cont/ack={ack_flag}, len={payload_len}")
        if ( data_flag==1 and ack_flag==1 and payload_len==0 and seq_num == self.ack_num ):
            print(f"Got ack packet, current state = {self.state}")
            if self.state == "WAIT_ACK":
                print(f"Got ack packet I needed, go back to idle")
                self.state = "IDLE"

    def is_tx_wait_ack(self):
        return self.state == "WAIT_ACK" 

    def is_tx_idle(self):
        if len(self.last_tx_packet) == 0 and self.state =="IDLE":
            return True
        return False



# Define some simple primitive operations, one PWM_RX per DPLL
class PWM_RX():
    def __init__(self, board):
        self.board = board
        self.MAX_PAYLOAD_SIZE = 4
        self.last_read = []
        # disable all decoders
        for i in range(len(self.board.dpll.modules["PWMDecoder"].BASE_ADDRESSES)):
            self.board.dpll.modules["PWMDecoder"].write_field(i,
                                                              "PWM_DECODER_CMD", "ENABLE", 0)

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




    def read_packet(self):
        hw_data = self.read_hardware_buffer()
        if len(hw_data):
            hw_data.reverse()
            header = hw_data[0]
            if ( header & 0x80 ): # data packet flag
                payload_len = (header >> 4) & 0x7
                return [header], hw_data[1:payload_len+1]
        else:
            return [], []
        return [], []

