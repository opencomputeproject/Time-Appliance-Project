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
TOD_SEC[5][4] = Slave following flag, tells the other side that the slave
    has started to follow TOD, valid with or without Data Flag
TOD_SEC[5][0:3] = Transaction ID
    0x0 = Read chip info
            a. Status of all inputs, STATUS.INX_MON_STATUS bytes for 0-15 (16 bytes)
            b. DPLL Status of DPLLs , STATUS.DPLLX_STATUS bytes for 0-3 (4 bytes)
            c. Input frequency monitor info, STATUS.INX_MON_FREQ_STATUS for 0-15, (32 bytes)
            d. A name string, 16 bytes including null
            e. TOD delta seen between received TOD frame and local TOD counter, used for round trip calculations (11 bytes)
            f. TOD delta sign, one byte

    0x1 = Write to board
            a. LED values bit-wise, 1 byte
            b. Force follow this requester, 1 byte, must be 0xa5 for this function, otherwise doesn't use
                Follow frequency and TOD and PPS
            c. The TOD offset seen on receiver at far side, for round trip compensation on follower side

TOD_SEC[4][7:0] = Random value for winner / loser determination, PWM User data state from loser upon end state

"""


"""
RX is the most constrained , so it needs the most logic

1. Need to round robin through decoders often
2. Want to detect
    a. is there a PWM encoder on the other side
    b. is the PWM encoder on the other side trying to initiate a request
"""

class AverageFilter:
    def __init__(self):
        self.values = []

    def update(self, value):
        self.values.append(value)

    def get_count(self):
        return len(self.values)

    def get_average(self, clear_avg=False):
        if ( len(self.values) ):
            avg = sum(self.values) / len(self.values)
            if ( clear_avg ):
                self.values = []
            return avg
        return 0

# basically control TOD , a single PWM Encoder, and a single decoder
# Secondary feature of this,
# Record TOD at reception of PPS from this decoder on each TOD using SecondaryTODRead
class dpof_single_channel():
    IDLE = 0
    RX_SLAVE = 1
    TRANSMIT_START = 2
    TRANSMIT_WON = 3
    TRANSMIT_WRITE = 4
    TRANSMIT_QUERY = 5
    TRANSMIT_DONE_WAIT = 6
    RX_SLAVE_RESPOND_QUERY = 7
    RX_SLAVE_RESPOND_QUERY_WAIT_FIFO_TX = 8
    RX_SLAVE_RESPOND_QUERY_WAIT_FIFO_TX_DONE = 9
    RX_SLAVE_WAIT_WRITE = 10
    RX_SLAVE_DONE_WAIT = 11

    def __init__(self, board, tod_num=0, encoder_num=0, decoder_num=0, decoder_time_slice_sec=5, DEBUG_PRINT=False):
        self.DEBUG_PRINT = DEBUG_PRINT
        self.board = board

        # tx variables
        self.tod_num = tod_num
        self.encoder = encoder_num
        self.tx_rand_num = 0
        self.tx_enabled = False
        self.time_tx_enabled = False
        self.current_transaction_id_tx = 0
        self.fifo_to_send = []

        # rx variables
        self.decoder = decoder_num
        # by default disable decoder
        # Probably shouldn't hard disable, just change ID
        #self.board.dpll.modules["PWMDecoder"].write_field(self.decoder,
        #                                                  "PWM_DECODER_CMD", "ENABLE", 0)

        self.rx_enabled = False
        self.decoder_time_slice_sec = decoder_time_slice_sec
        self.last_data_this_decoder = []

        self.fifo_grant = False
        self.state = dpof_single_channel.IDLE

        # store what got written to me
        self.pwm_write_data = []

        # store what I read back
        self.pwm_query_data = []

        # store latest TOD frame data (not including handshake bytes)
        # first entry is what was received over PWM, other 4 entries are each local TOD
        # counter value at time of PWM reception
        self.tod_compare_data = []

        self.follow_far_side = False

        self.far_side_following = False

        self.last_tod_push = []


    def grant_fifo_control(self):
        self.fifo_grant = True

    def release_fifo_control(self):
        self.fifo_grant = False

    def get_fifo_grant_status(self):
        return self.fifo_grant


    def set_follow_far_side(self):
        if ( self.follow_far_side ):
            return # already following

        # read back current TOD
        cur_tod = self.read_current_tx_tod_seconds()
        print(f"Set follow far side, got tod {cur_tod}")

        # flip the order
        cur_tod.reverse()

        # set the bit
        jump_sec = (cur_tod[0] | (1<<4)) << (8*5)

        # relative jump positive
        self.board.dpof.write_tod_relative(
            self.encoder, 0, 0, jump_sec, True)

        # set the flag
        self.follow_far_side = True


    def stop_follow_far_side(self):
        if ( not self.follow_far_side ):
            return # already not following
        # read back current TOD
        cur_tod = self.read_current_tx_tod_seconds()

        # flip the order
        cur_tod.reverse()

        # clear the bit
        jump_sec = (cur_tod[0] & 0xef) << (8*5)

        # relative jump negative
        self.board.dpof.write_tod_relative(
            self.encoder, 0, 0, jump_sec, False)

        # set the flag
        self.follow_far_side = False



    def run_idle_state(self):
        # only transition out of idle is if RX data detected
        # TX transaction is "async"
        data = self.check_decoder_new_data()
        if (len(data) == 0):
            # Case 1: Decoder inactive
            return self.disable_decoder_if_time_over(), 0
        else:
            # Case 2: Decoder active
            # check decoder value
            top_byte = data[-1]
            rand_id_rx = data[-2]
            data_flag = (top_byte >> 7) & 0x1
            handshake_state_rx = (top_byte >> 5) & 0x3
            transaction_id_rx = top_byte & 0xf
            if (not data_flag):
                return self.disable_decoder_if_time_over(), False
            else:
                self.state = dpof_single_channel.RX_SLAVE
                self.master_request = transaction_id_rx
                if (self.DEBUG_PRINT):
                    print(
                        f"Going to rx slave state from idle, master request = 0x{self.master_request:02x}")
                return self.disable_decoder_if_time_over(), True

    def run_rx_slave_state(self):
        # only transition out of this state is if fifo grant is granted by higher level
        # or TX API called
        if (self.fifo_grant):
            if (self.DEBUG_PRINT):
                print(f"RX slave state got fifo grant")
            # got the grant, now I can use PWM FIFO for dpll over fiber as slave
            if (self.is_transaction_id_query(self.master_request)):
                # write 0x1 to master
                self.start_tx(0x1, self.master_request, [])

                # master request is a query, don't need fifo or decoder locked yet
                self.state = dpof_single_channel.RX_SLAVE_RESPOND_QUERY

                if (self.DEBUG_PRINT):
                    print(f"RX slave state go to respond query")
                return self.disable_decoder_if_time_over(), True
            else:

                # set fifo to receive, 0x0 for idle
                self.board.dpll.modules["PWM_USER_DATA"].write_reg(0,
                                                                   "PWM_USER_DATA_PWM_USER_DATA_CMD_STS", 0x0)

                # write 0x1 to master
                self.start_tx(0x1, self.master_request, [])

                # master request is a write
                # NEED TO HOLD FIFO AND DECODER
                self.state = dpof_single_channel.RX_SLAVE_WAIT_WRITE

                if (self.DEBUG_PRINT):
                    print(f"RX slave state go to wait write")
                return True, True
        return self.disable_decoder_if_time_over(), True

    def run_rx_slave_respond_query(self):
        # only a wait state, wait for 0x1 from master
        data = self.check_decoder_new_data()
        if (len(data) == 0):
            # Case 1: Decoder inactive
            return self.disable_decoder_if_time_over(), False
        else:
            # Case 2: Decoder active
            # check decoder value
            top_byte = data[-1]
            rand_id_rx = data[-2]
            data_flag = (top_byte >> 7) & 0x1
            handshake_state_rx = (top_byte >> 5) & 0x3
            transaction_id_rx = top_byte & 0xf
            if (self.DEBUG_PRINT):
                print(
                    f"Run rx slave respond query handshake_state_rx id={handshake_state_rx}")

            if (handshake_state_rx == 0x1):
                # Got 0x1, write to PWM FIFO and send it, wait for TX Completion on FIFO
                self.fifo_to_send = self.get_fifo_respond_to_query(
                    transaction_id_rx)
                if (self.DEBUG_PRINT):
                    print(f"Will send {len(self.fifo_to_send)} bytes of PWM")

                self.board.dpll.modules["PWM_USER_DATA"].write_reg(0,
                                                                   "PWM_USER_DATA_PWM_USER_DATA_SIZE", len(self.fifo_to_send))

                self.board.dpll.modules["PWM_USER_DATA"].write_reg(0,
                                                                   "PWM_USER_DATA_PWM_USER_DATA_CMD_STS", 0x1)  # send transmission request

                # wait for TX completion on FIFO
                self.state = dpof_single_channel.RX_SLAVE_RESPOND_QUERY_WAIT_FIFO_TX

                if (self.DEBUG_PRINT):
                    print(f"Go to rx slave respond query wait fifo tx")

                return self.disable_decoder_if_time_over(), True
        return self.disable_decoder_if_time_over(), True

    def run_rx_slave_respond_query_wait_fifo_tx(self):
        # only a wait state, wait for PWM FIFO to say TX completed or errored or something

        fifo_status = self.board.dpll.modules["PWM_USER_DATA"].read_reg(0,
                                                                        "PWM_USER_DATA_PWM_USER_DATA_CMD_STS")
        if (fifo_status == 0x3):  # got tx ack , can send data now

            if (self.DEBUG_PRINT):
                print(f"RX slave respond query wait fifo tx, got tx ack")
                print(f" Sending fifo {self.fifo_to_send}")
            for i in range(len(self.fifo_to_send)):
                reg_name = f"BYTE_OTP_EEPROM_PWM_BUFF_{i}"
                #print(f"RX Slave respond query wait fifo write i={i} {self.fifo_to_send}")
                self.board.dpll.modules["EEPROM_DATA"].write_reg(0,
                                                                 reg_name, self.fifo_to_send[i])

            # start transmission
            self.board.dpll.modules["PWM_USER_DATA"].write_reg(0,
                                                               "PWM_USER_DATA_PWM_USER_DATA_CMD_STS", 0x2)  # sent transmission request

            self.state = dpof_single_channel.RX_SLAVE_RESPOND_QUERY_WAIT_FIFO_TX_DONE
        elif (fifo_status > 0x3):
            if (self.DEBUG_PRINT):
                print(f"GOT FIFO STATUS BAD, {fifo_status}")

        # don't need decoder, but need PWM fifo
        return self.disable_decoder_if_time_over(), True

    def run_rx_slave_respond_query_wait_fifo_tx_done(self):
        # simple wait state, wait for PWM FIFO to send it sent out the data
        fifo_status = self.board.dpll.modules["PWM_USER_DATA"].read_reg(0,
                                                                        "PWM_USER_DATA_PWM_USER_DATA_CMD_STS")

        if (fifo_status == 0x5):
            # once complete, send 0x2 and go to done wait
            if (self.DEBUG_PRINT):
                print(f"Fifo status good, PWM transmission successful!")
            self.start_tx(0x2, self.master_request, [])
            self.state = dpof_single_channel.RX_SLAVE_DONE_WAIT

            # done with FIFO
            return self.disable_decoder_if_time_over(), False
        else:
            if (self.DEBUG_PRINT):
                print(f"Fifo status not done yet, respond query wait fifo tx done")
        return self.disable_decoder_if_time_over(), True

    def run_rx_slave_done_wait(self):
        # sent 0x2, wait for data flag to go away or state to change to 0x0
        data = self.check_decoder_new_data()
        if (len(data) == 0):
            # Case 1: Decoder inactive
            return self.disable_decoder_if_time_over(), 0
        else:
            # Case 2: Decoder active
            # check decoder value
            print(f"Run rx slave done wait, got data {data}")
            top_byte = data[-1]
            rand_id_rx = data[-2]
            data_flag = (top_byte >> 7) & 0x1
            handshake_state_rx = (top_byte >> 5) & 0x3
            transaction_id_rx = top_byte & 0xf
            if (not data_flag):
                self.state = dpof_single_channel.IDLE
                if (self.DEBUG_PRINT):
                    print(f"RX slave done wait state, going to idle")
                self.stop_tx()
            else:
                if (handshake_state_rx == 0x0):
                    self.state = dpof_single_channel.RX_SLAVE
                    self.master_request = transaction_id_rx
                    if (self.DEBUG_PRINT):
                        print(f"RX slave done wait state, going to rx slave")
                    return self.disable_decoder_if_time_over(), True
        return self.disable_decoder_if_time_over(), False

    def run_rx_slave_wait_write(self):
        # wait for PWM FIFO to fill up
        fifo_status = self.board.dpll.modules["PWM_USER_DATA"].read_reg(0,
                                                                        "PWM_USER_DATA_PWM_USER_DATA_CMD_STS")
        if (fifo_status == 0xb):
            if (self.DEBUG_PRINT):
                print(f"PWM User data reception successful!")

            fifo_byte_count = self.board.dpll.modules["PWM_USER_DATA"].read_reg(0,
                                                                                "PWM_USER_DATA_PWM_USER_DATA_SIZE")
            if (self.DEBUG_PRINT):
                print(
                    f"Slave wait write Received {fifo_byte_count} through FIFO")

            fifo_data = []

            for i in range(fifo_byte_count):
                reg_name = f"BYTE_OTP_EEPROM_PWM_BUFF_{i}"
                fifo_data.append(self.board.dpll.modules["EEPROM_DATA"].read_reg(0,
                                                                                 reg_name))

            if (self.DEBUG_PRINT):
                print(f"Slave wait write Received fifo data: {fifo_data}")

            self.pwm_write_data.append(
                [self.decoder, self.master_request, fifo_data])

            # send 0x2 and wait
            self.start_tx(0x2, self.master_request, [])
            self.state = dpof_single_channel.RX_SLAVE_DONE_WAIT
            if (self.DEBUG_PRINT):
                print(f"Going to RX slave done wait")
        else:
            if (self.DEBUG_PRINT):
                print(
                    f"PWM User data reception not done yet! {fifo_status:02x}")

        # need to hold decoder and fifo
        return True, True

    def run_transmit_start_state(self):
        # come to this state async when transmit is started from idle state
        # check the RX data if data flag detected
        data = self.check_decoder_new_data()
        if (len(data) == 0):
            # Case 1: Decoder inactive
            return self.disable_decoder_if_time_over(), False
        else:
            # Case 2: Decoder active
            # check decoder value
            top_byte = data[-1]
            rand_id_rx = data[-2]
            data_flag = (top_byte >> 7) & 0x1
            handshake_state_rx = (top_byte >> 5) & 0x3
            transaction_id_rx = top_byte & 0xf
            if (data_flag):
                if (handshake_state_rx == 0x1):
                    # I sent out 0x0, got back 0x1, I won negotiation, need FIFO
                    self.state = dpof_single_channel.TRANSMIT_WON
                    return self.disable_decoder_if_time_over(), True
                elif (handshake_state_rx == 0x0):
                    # I sent out 0x0, but also go back 0x0, negotiation
                    # check random numbers
                    if (self.tx_rand_num < rand_id_rx):
                        # I win, need FIFO now
                        self.state = dpof_single_channel.TRANSMIT_WON
                        return self.disable_decoder_if_time_over(), True
                    elif (self.tx_rand_num > rand_id_rx):
                        # I lose, but HOW DO I TRACK THE TX DATA?????
                        # for now whatever, query is discarded
                        self.master_request = transaction_id_rx
                        self.state = dpof_single_channel.RX_SLAVE
                        self.stop_tx()
                    else:
                        # restart negotiation, just force start TX again
                        self.start_tx(self.tx_nego_state,
                                      self.current_transaction_id_tx,
                                      self.fifo_to_send)

        # don't need fifo or decoder at this point
        return self.disable_decoder_if_time_over(), False

    def run_transmit_won_state(self):

        # don't proceed or do anything in this state without FIFO lock
        if (not self.fifo_grant):
            return self.disable_decoder_if_time_over(), True

        if (self.DEBUG_PRINT):
            print(f"Transmit won state got fifo grant")
        if (self.is_tx_query):  # I'm trying to query and have FIFO lock
            # enable PWM FIFO for reception
            # set fifo to receive, 0x0 for idle
            self.board.dpll.modules["PWM_USER_DATA"].write_reg(0,
                                                               "PWM_USER_DATA_PWM_USER_DATA_CMD_STS", 0x0)

            # send 0x1 to let other side know I'm ready to receive
            self.start_tx(0x1, self.current_transaction_id_tx, [])
            self.state = dpof_single_channel.TRANSMIT_QUERY
            # need to hold both decoder and FIFO
            if (self.DEBUG_PRINT):
                print(f"Transmit won state, going to transmit query")
            return True, True
        else:
            # I'm trying to write to other end
            # Got 0x1, write to PWM FIFO and send it, wait for TX Completion on FIFO
            self.board.dpll.modules["PWM_USER_DATA"].write_reg(0,
                                                               "PWM_USER_DATA_PWM_USER_DATA_SIZE", len(self.fifo_to_send))

            if (self.DEBUG_PRINT):
                print(f"Going to transmit {len(self.fifo_to_send)} bytes")

            self.board.dpll.modules["PWM_USER_DATA"].write_reg(0,
                                                               "PWM_USER_DATA_PWM_USER_DATA_CMD_STS", 0x1)  # send transmission request
            self.state = dpof_single_channel.TRANSMIT_WRITE

            if (self.DEBUG_PRINT):
                print(f"Transmit won state, going to transmit write")

        # need FIFO
        return self.disable_decoder_if_time_over(), True

    def run_transmit_write_state(self):
        # assume fifo grant, without it wont get to this state
        # pseudo wait state, waiting for CMD_STS to
        pwm_status = self.board.dpll.modules["PWM_USER_DATA"].read_reg(0,
                                                                       "PWM_USER_DATA_PWM_USER_DATA_CMD_STS")

        if (self.DEBUG_PRINT):
            print(f"Transmit write state, check PWM user status, {pwm_status}")
        if (pwm_status == 0x3):  # got tx ack, can send data now
            for i in range(len(self.fifo_to_send)):
                reg_name = f"BYTE_OTP_EEPROM_PWM_BUFF_{i}"
                self.board.dpll.modules["EEPROM_DATA"].write_reg(0,
                                                                 reg_name, self.fifo_to_send[i])

            # start transmission
            self.board.dpll.modules["PWM_USER_DATA"].write_reg(0,
                                                               "PWM_USER_DATA_PWM_USER_DATA_CMD_STS", 0x2)  # sent transmission request

            if (self.DEBUG_PRINT):
                print(f"Got tx ack, sending data now! Going to transmit done wait state")

            # don't necessarily need to wait for TX FIFO to say it's done, just wait for other side
            self.state = dpof_single_channel.TRANSMIT_DONE_WAIT

        return self.disable_decoder_if_time_over(), True

    def run_transmit_query_state(self):
        # assume fifo grant
        # wait for CMD_STS to say something about receiver
        # wait for PWM FIFO to fill up
        fifo_status = self.board.dpll.modules["PWM_USER_DATA"].read_reg(0,
                                                                        "PWM_USER_DATA_PWM_USER_DATA_CMD_STS")

        if (self.DEBUG_PRINT):
            print(f"Run transmit query state, fifo status {fifo_status:02x}")

        if (fifo_status == 0xb):

            if (self.DEBUG_PRINT):
                print(f"PWM User data reception successful!")

            fifo_byte_count = self.board.dpll.modules["PWM_USER_DATA"].read_reg(0,
                                                                                "PWM_USER_DATA_PWM_USER_DATA_SIZE")

            if (self.DEBUG_PRINT):
                print(f"Received {fifo_byte_count} through FIFO")

            fifo_data = []

            for i in range(fifo_byte_count):
                reg_name = f"BYTE_OTP_EEPROM_PWM_BUFF_{i}"
                fifo_data.append(self.board.dpll.modules["EEPROM_DATA"].read_reg(0,
                                                                                 reg_name))

            if (self.DEBUG_PRINT):
                print(f"Received fifo data: {fifo_data}")

            self.pwm_query_data.append([self.decoder, self.current_transaction_id_tx,
                                        fifo_data])

            # check if RX already sent 0x2, if not go to wait state
            data = self.check_decoder_new_data()
            if (len(data) == 0):
                # Case 1: Decoder inactive
                self.state = dpof_single_channel.TRANSMIT_DONE_WAIT
                return self.disable_decoder_if_time_over(), False
            else:
                # Case 2: Decoder active
                # check decoder value
                top_byte = data[-1]
                rand_id_rx = data[-2]
                data_flag = (top_byte >> 7) & 0x1
                handshake_state_rx = (top_byte >> 5) & 0x3
                transaction_id_rx = top_byte & 0xf
                if (handshake_state_rx == 0x2):  # got ack from other side
                    if (self.DEBUG_PRINT):
                        print(f"Transmit query state got 0x2, done")
                    self.stop_tx()
                    self.state = dpof_single_channel.IDLE
                    return self.disable_decoder_if_time_over(), False

            # go to this state to wait for 0x2
            self.state = dpof_single_channel.TRANSMIT_DONE_WAIT
            return self.disable_decoder_if_time_over(), False
        else:
            if (self.DEBUG_PRINT):
                print(
                    f"PWM User data reception not done yet! {fifo_status:02x}")

        # need to hold decoder and fifo
        return True, True

    def run_transmit_done_wait_state(self):
        # debug, read cmd status
        fifo_status = self.board.dpll.modules["PWM_USER_DATA"].read_reg(0,
                                                                        "PWM_USER_DATA_PWM_USER_DATA_CMD_STS")

        if (self.DEBUG_PRINT):
            print(
                f"Run transmit done wait state debug fifo-status {fifo_status:02x}")

        # need to wait for rx data
        data = self.check_decoder_new_data()
        if (len(data) == 0):
            # Case 1: Decoder inactive
            return self.disable_decoder_if_time_over(), False
        else:
            # Case 2: Decoder active
            # check decoder value
            top_byte = data[-1]
            rand_id_rx = data[-2]
            data_flag = (top_byte >> 7) & 0x1
            handshake_state_rx = (top_byte >> 5) & 0x3
            transaction_id_rx = top_byte & 0xf
            if (handshake_state_rx == 0x2):  # got ack from other side
                if (self.DEBUG_PRINT):
                    print(f"Transmit done wait state got 0x2, done")
                self.stop_tx()
                self.state = dpof_single_channel.IDLE

        return self.disable_decoder_if_time_over(), False

    # returns [bool, bool] value
    # First bool, whether decoder is now disabled or enabled, True for enabled
    # Second bool, if PWM FIFO is needed
    def top_state_machine(self):
        if (self.DEBUG_PRINT):
            print(
                f"Board {self.board.board_num} top state machine channel {self.tod_num}")
        if (self.state == dpof_single_channel.IDLE):
            if (self.DEBUG_PRINT):
                print(f"Running idle state")
            return self.run_idle_state()
        elif (self.state == dpof_single_channel.RX_SLAVE):
            if (self.DEBUG_PRINT):
                print(f"Running rx slave state")
            return self.run_rx_slave_state()
        elif (self.state == dpof_single_channel.RX_SLAVE_RESPOND_QUERY):
            if (self.DEBUG_PRINT):
                print(f"Running run_rx_slave_respond_query")
            return self.run_rx_slave_respond_query()
        elif (self.state == dpof_single_channel.RX_SLAVE_RESPOND_QUERY_WAIT_FIFO_TX):
            if (self.DEBUG_PRINT):
                print(
                    f"Running run_rx_slave_respond_query_wait_fifo_tx")
            return self.run_rx_slave_respond_query_wait_fifo_tx()
        elif (self.state == dpof_single_channel.RX_SLAVE_RESPOND_QUERY_WAIT_FIFO_TX_DONE):
            if (self.DEBUG_PRINT):
                print(
                    f"Running run_rx_slave_respond_query_wait_fifo_tx_done")
            return self.run_rx_slave_respond_query_wait_fifo_tx_done()
        elif (self.state == dpof_single_channel.RX_SLAVE_WAIT_WRITE):
            if (self.DEBUG_PRINT):
                print(f"Running run_rx_slave_wait_write")
            return self.run_rx_slave_wait_write()
        elif (self.state == dpof_single_channel.RX_SLAVE_DONE_WAIT):
            if (self.DEBUG_PRINT):
                print(f"Running run_rx_slave_done_wait")
            return self.run_rx_slave_done_wait()
        elif (self.state == dpof_single_channel.TRANSMIT_START):
            if (self.DEBUG_PRINT):
                print(f"Running transmit start state")
            return self.run_transmit_start_state()
        elif (self.state == dpof_single_channel.TRANSMIT_WON):
            if (self.DEBUG_PRINT):
                print(f"Running transmit won state")
            return self.run_transmit_won_state()
        elif (self.state == dpof_single_channel.TRANSMIT_WRITE):
            if (self.DEBUG_PRINT):
                print(f"Running transmit write state")
            return self.run_transmit_write_state()
        elif (self.state == dpof_single_channel.TRANSMIT_QUERY):
            if (self.DEBUG_PRINT):
                print(f"Running transmit query state")
            return self.run_transmit_query_state()
        elif (self.state == dpof_single_channel.TRANSMIT_DONE_WAIT):
            if (self.DEBUG_PRINT):
                print(f"Running transmit done wait state")
            return self.run_transmit_done_wait_state()
        return self.disable_decoder_if_time_over(), False


################ TX FUNCTIONS ####################


    def can_tx(self):
        if (self.state == dpof_single_channel.IDLE):
            return True
        return False

    def start_tx(self, handshake_id=0, transaction_id=0, fifo_data=[]):
        if (self.tx_enabled == True):  # already enabled
            self.stop_tx()

        self.tx_rand_num = random.randint(0, 255)
        tod_sec_top = (1 << 7) + ((handshake_id & 0x3) << 5) + \
            (transaction_id & 0xf)
        tod_sec_bot = self.tx_rand_num
        tod_to_jump = (tod_sec_top << (8*5)) + (tod_sec_bot << (8*4))
        # relative jump positive
        print(f"Start tx, write tod_relative encoder {self.encoder} tod_to_jump={tod_to_jump}")
        self.board.dpof.write_tod_relative(self.encoder, 0, 0, tod_to_jump, True)
        self.tx_enabled = True
        self.time_tx_enabled = time.time()
        self.tx_nego_state = handshake_id
        self.current_transaction_id_tx = transaction_id
        self.fifo_to_send = fifo_data
        self.is_tx_query = self.is_transaction_id_query(transaction_id)
        self.state = dpof_single_channel.TRANSMIT_START

        if (self.DEBUG_PRINT):
            print(
                f"Board {self.board.board_num} transmit start id={handshake_id} trans_id={transaction_id} rand={self.tx_rand_num} data {fifo_data}")
        return True

    def stop_tx(self):
        if (not self.tx_enabled):
            return

        # read back current TOD
        cur_tod = self.read_current_tx_tod_seconds()

        # flip the order
        cur_tod.reverse()

        if (cur_tod[0] & 0x80):  # data bit is set , I need to clear it
            # keep bit 4 of upper seconds
            cur_tod_to_sub = ( (cur_tod[0] & 0xef) << (8*5)) + ( cur_tod[1] << (8*4))
            # relative jump negative
            self.board.dpof.write_tod_relative(
                self.encoder, 0, 0, cur_tod_to_sub, False)
        self.tx_enabled = False
        self.tx_nego_state = 0

################ RX FUNCTIONS ###################

    def is_transaction_id_query(self, query_id):
        # print(f"RX Transaction ID query!")
        if (query_id == 0):
            return True
        return False

    # when I lose handshake, and far side is querying me

    def get_fifo_respond_to_query(self, query_id):
        if (self.DEBUG_PRINT):
            print(f"RX Fill fifo respond to query {query_id}")
        if (query_id == 0):
            # 0x0 = Read chip info
            #        a. Status of all inputs, STATUS.INX_MON_STATUS bytes for 0-15 (16 bytes)
            #        b. DPLL Status of DPLLs , STATUS.DPLLX_STATUS bytes for 0-3 (4 bytes)
            #        c. Input frequency monitor info, STATUS.INX_MON_FREQ_STATUS for 0-15, (32 bytes)
            #        d. A name string, 16 bytes including null
            #        e. TOD delta seen between local TOD counter and received TOD frame , used for round trip calculations (11 bytes)
            #        f. 1 byte, represents if this TOD was positive or negative with respect to local TOD,
            # 1 for positive meaning local TOD was > remote TOD , 0 for negative meaning local TOD was < remote TOD
            # g. 1 byte, clock quality, 255 is worst, 0 is best

            status_bytes = []
            dpll_status_bytes = []
            input_freq_mon_bytes = []
            tod_delta = [0] * 11
            tod_flag = 0
            string = f"MiniPTM{self.board.board_num}"
            id_bytes = []
            for i in range(16):
                status_bytes.append(self.board.dpll.modules["Status"].read_reg(0,
                                                                               f"IN{i}_MON_STATUS"))

            for i in range(4):
                dpll_status_bytes.append(self.board.dpll.modules["Status"].read_reg(0,
                                                                                    f"DPLL{i}_STATUS"))

            for i in range(16):
                input_freq_mon_bytes += self.board.dpll.modules["Status"].read_reg_mul(0,
                                                                                       f"IN{i}_MON_FREQ_STATUS_0", 2)

            id_bytes = [ord(ch) for ch in string]
            id_bytes += [0] * (16 - len(id_bytes))

           # TOD delta, get the latest TOD value and use self.decoder_num / 2

            if (len(self.tod_compare_data) > 0):
                incoming_tod = self.tod_compare_data[0][1]  # the incoming TOD
                val = int(self.decoder / 2) + 2
                print(f"Decoder {val} tod_compare_data={self.tod_compare_data}")
                local_tod = self.tod_compare_data[0][val]
                difference, flag = time_difference_with_flag(
                    local_tod, incoming_tod, True)
                tod_delta = difference + [0,0]

                # flag is 1 if time1 > time 2 , -1 if time1 < time2 , 0 if equal
                if (flag == 1):
                    # local_tod > incoming_tod , 1
                    tod_flag = 1
                else:
                    # incoming_tod > local_tod , 0
                    tod_flag = 0

            clock_quality = 255 - self.board.board_num
            to_return = status_bytes + dpll_status_bytes + input_freq_mon_bytes 
            to_return += id_bytes + tod_delta + [tod_flag] + [clock_quality]

            print(
                f"FIFO respond to query board {self.board.board_num} , {to_return}")
            return to_return
            # big query
        else:
            pass
            # undefined
        return []

    # returns False if decoder disabled, true if still enabled
    def disable_decoder_if_time_over(self):
        if (self.get_how_long_decoder_on() >= self.decoder_time_slice_sec):
            if (self.DEBUG_PRINT):
                print(
                    f"Stopping RX, {self.get_how_long_decoder_on()}, {self.decoder_time_slice_sec}")
            # time slice elapsed
            self.stop_rx()
            return False
        elif (not self.rx_enabled):
            return False
        return True

    def stop_rx(self):
        if (self.DEBUG_PRINT):
            print(f"Board {self.board.board_num} stop rx {self.decoder}")
        # simple, turn off decoder
        self.board.dpll.modules["PWMDecoder"].write_field(self.decoder,
                                                          "PWM_DECODER_CMD", "ENABLE", 0)
        self.rx_enable = False

    def start_rx(self):
        if (self.DEBUG_PRINT):
            print(f"Board {self.board.board_num} start rx {self.decoder}")
        # record what PWM TOD is to detect change after enabling decoder
        self.pwm_tod_before_start_rx = self.read_raw_hardware_buffer()

        self.last_tod_push = []

        # Secondary feature, record TOD at reception of PPS from this decoder
        # on each TOD using SecondaryTODRead

        # clear any previous triggers
        for i in range(4):  # 4 TODs

            if (self.DEBUG_PRINT):
                print(
                    f"Enable TOD Secondary {i} read using decoder {self.decoder}")

            self.board.dpll.modules["TODReadSecondary"].write_field(i,
                                                                    "TOD_READ_SECONDARY_CMD", "TOD_READ_TRIGGER", 0x0)

            # configure this decoder as trigger source
            self.board.dpll.modules["TODReadSecondary"].write_field(i,
                                                                    "TOD_READ_SECONDARY_SEL_CFG_0", "PWM_DECODER_INDEX", self.decoder)

            # configure continous TOD read on PWM Decoder 1PPS output
            self.board.dpll.modules["TODReadSecondary"].write_reg(i,
                                                                  "TOD_READ_SECONDARY_CMD", 0x14)

        # enable decoder with frame access
        self.board.dpll.modules["PWMDecoder"].write_field(self.decoder,
                                                          "PWM_DECODER_CMD", "ENABLE", 0x5)

        # record time when started decoder
        self.start_time_on_this_decoder = time.time()
        self.rx_enabled = True

    def get_how_long_decoder_on(self):
        if (self.rx_enabled):
            return time.time() - self.start_time_on_this_decoder
        return 0


    def push_tod_compare_data(self, data):
        tod_compare = []
        tod_compare.append(self.decoder)
        # ignore top two bytes, handshake bytes
        tod_compare.append(data[:-2])
        
        local_tod_save = []

        for i in range(4):  # read back TODs as well
            local_tod = self.board.dpll.modules["TODReadSecondary"].read_reg_mul(i,
                     "TOD_READ_SECONDARY_SUBNS", 11)
            local_tod = local_tod[:-2]
            tod_compare.append(local_tod)
            if ( i == self.decoder/2 ):
                local_tod_save = list(local_tod)

        if ( data[-1] & 0x10 ): # slave follow flag
            print(f"Decoder {self.decoder} far side is following!")
            self.far_side_following = True
            
            # far side is following me
            # keep an average of TOD values seen
            # used for round trip estimation
            remote_tod = data[:-2]
            local_tod = local_tod_save

            tod_diff = time_difference_signed_nanoseconds(local_tod, remote_tod,
                    True)

        else:
            self.far_side_following = False

        if (self.DEBUG_PRINT):
            print(f"Board {self.board.board_num} Debug tod compare data {tod_compare}")
        self.tod_compare_data = []
        self.tod_compare_data.append(tod_compare)



    def check_decoder_new_data(self):
        if (self.rx_enabled):
            data = self.read_raw_hardware_buffer(True)
            # only keep handshake bytes, top two bytes
            data_hs = data[-2:]
            # non handshake bytes, "real" tod
            data_nonhs = data[:-2]

            pwm_stale = data_hs == self.pwm_tod_before_start_rx[-2:]
            pwm_tod_stale = data_nonhs == self.last_tod_push 

            if self.state == dpof_single_channel.RX_SLAVE_DONE_WAIT:
                pwm_stale = False # can't be stale, just went through full data

            if ((pwm_stale) or
                    (data_hs == self.last_data_this_decoder)):
                print(f"Not new decoder data, pwm_stale={pwm_stale} ")

                if ( not pwm_tod_stale ): 
                    # got new data, but not new handshake data
                    #this is fine for TOD compare
                    print(f"Pushing tod compare even though no new decoder data")
                    self.push_tod_compare_data(data)
                    self.last_tod_push = data_nonhs

                # nothing new
                return []
            else:
                hex_val = [hex(val) for val in data_hs]
                if (self.DEBUG_PRINT):
                    print(
                        f" Debug check decoder new data {data_hs} {self.last_data_this_decoder}")
                    print(
                        f"Board {self.board.board_num} decoder {self.decoder} new data {hex_val}")
                self.last_data_this_decoder = list(data_hs)
                if (self.DEBUG_PRINT):
                    print(f" Debug {self.last_data_this_decoder}")

                # Secondary feature, record TODs from TODReadSecondary and this PWM frame
                self.push_tod_compare_data(data)
                self.last_tod_push = data_nonhs
                return data_hs
        return []

    # read current TOD encoder value, only two handshake bytes

    def read_current_tx_tod_seconds(self):
        # use read primary
        self.board.dpll.modules["TODReadPrimary"].write_reg(self.tod_num,
                                                            "TOD_READ_PRIMARY_CMD", 0x0)
        self.board.dpll.modules["TODReadPrimary"].write_reg(self.tod_num,
                                                            "TOD_READ_PRIMARY_CMD", 0x1)

        cur_tod = self.board.dpll.modules["TODReadPrimary"].read_reg_mul(self.tod_num,
                                                                         "TOD_READ_PRIMARY_SECONDS_32_39", 2)

        return cur_tod

    # returns data from hardware buffer, only two handshake bytes
    def read_raw_hardware_buffer(self, full_data=False):
        """ Read data from the hardware's global receive buffer. """
        # just read seconds portion
        data = self.board.i2c.read_dpll_reg_multiple(0xce80, 0x0, 11)
        hex_val = [hex(val) for val in data]

        if (self.DEBUG_PRINT):
            print(
                f"Read PWM Raw Receive Board {self.board.board_num} TOD {hex_val}")

        if (full_data):
            return data
        return data[-2:]


#####################################################################
# Top level dpll over fiber class for one MiniPTM / DPLL over fiber DPLL instnace

class DPOF_Top():
    def __init__(self, board, DEBUG_PRINT = True):
        self.board = board

        # NEED TO CHANGE THIS, probably just mess with decoder IDs
        # disable all decoders
        #for i in range(len(self.board.dpll.modules["PWMDecoder"].BASE_ADDRESSES)):
        #    self.board.dpll.modules["PWMDecoder"].write_field(i,
        #                                                      "PWM_DECODER_CMD", "ENABLE", 0)

        # RX Logic variables
        self.active_decoder = 0
        self.fifo_lock_chan = -1

        # HUGE HACK, ONLY USE CHANNEL 0 FOR BENCHTOP DEBUG
        self.channels = [dpof_single_channel(
            self.board, i, i, i*2, 5) for i in range(4)]  # decoders skip one

        # enable one
        #self.channels[self.active_decoder].start_rx()

        # channels following me as master
        self.follow_channels = []

        # who I'm following as master if anyone
        self.master_decoder = -1
        self.following_master = False

        self.average_tod_errors = []
        for index in range(4):
            self.average_tod_errors.append( AverageFilter() )


        # TX Logic variables
        self.DEBUG_PRINT = DEBUG_PRINT

    # top level function

    def tick(self):
        for index, chan in enumerate(self.channels):
            if ( self.DEBUG_PRINT ):
                print(f"Board {self.board.board_num} DPOF tick, chan {index}")
            decoder_enabled, fifo_needed = chan.top_state_machine()
            if ( self.DEBUG_PRINT ):
                print(
                    f"Board {self.board.board_num} DPOF tick, chan {index}, {decoder_enabled} , {fifo_needed}")

            if (not decoder_enabled):  # it turned itself off
                if (index == self.active_decoder):  # it was enabled
                    # enable next one
                    self.active_decoder = (
                        self.active_decoder + 1) % len(self.channels)
                    if ( self.DEBUG_PRINT ):
                        print(
                            f"Board {self.board.board_num} switch to decoder {self.active_decoder}")
                    self.channels[self.active_decoder].start_rx()

            if (fifo_needed):  # it's requesting FIFO control
                if (self.fifo_lock_chan == -1):
                    chan.grant_fifo_control()
                    self.fifo_lock_chan = index

            if (not fifo_needed):  # it doesn't need FIFO
                if (self.fifo_lock_chan == index):  # it had fifo control
                    # release fifo lock
                    chan.release_fifo_control()
                    self.fifo_lock_chan = -1

            # check if any results
            if ( self.DEBUG_PRINT ):
                if (len(chan.pwm_write_data) > 0):
                    print(f"DPOF Top tick, got written to! {chan.pwm_write_data}")
                elif (len(chan.pwm_query_data) > 0):
                    print(
                        f"DPOF Top Tick, got query data back! {chan.pwm_query_data}")

    # returns a single query data entry, [channel, query_id, [query data]]
    # or returns empty list

    def pop_query_data(self):
        for index, chan in enumerate(self.channels):
            if (len(chan.pwm_query_data) > 0):
                data = chan.pwm_query_data.pop(0)
                return data
        return []

    def pop_write_data(self):
        for index, chan in enumerate(self.channels):
            if (len(chan.pwm_write_data) > 0):
                data = chan.pwm_write_data.pop(0)
                return data
        return []

    def get_chan_tx_ready(self, channel_num=0):
        return self.channels[channel_num].can_tx()

    def dpof_query(self, channel_num=0, query_id=0):
        if (self.channels[channel_num].can_tx()):
            if ( self.DEBUG_PRINT ):
                print(
                    f"Board {self.board.board_num} can TX, starting TX chan={channel_num} query_id={query_id}")
            self.channels[channel_num].start_tx(transaction_id=query_id)
            return True
        return False

    def dpof_write(self, channel_num=0, write_id=0, data=[]):
        if (self.channels[channel_num].can_tx()):
            self.channels[channel_num].start_tx(transaction_id=write_id,
                                                fifo_data=data)
            return True
        return False

    def get_tod_compare(self):
        data = []
        for index, chan in enumerate(self.channels):
            if (len(chan.tod_compare_data) > 0):
                val = chan.tod_compare_data.pop(0)
                print(f"Chan {index} had tod_compare_data {val}")
                data.append( val )
        return data

    def write_tod_absolute(self, tod_num=0, tod_subns=0, tod_ns=0, tod_sec=0):
        data = []
        data += [tod_subns & 0xff]
        data += [byte for byte in tod_ns.to_bytes(4, byteorder='little')]
        data += [byte for byte in tod_sec.to_bytes(6, byteorder='little')]
        # print(f"Write TOD Absolute addr 0x{addr:x} -> {data}")
        self.board.dpll.modules["TODWrite"].write_reg_mul(tod_num, "TOD_WRITE_SUBNS", 
                data)
        # write the trigger for immediate absolute
        self.board.dpll.modules["TODWrite"].write_reg(tod_num, "TOD_WRITE_CMD", 0x0)
        self.board.dpll.modules["TODWrite"].write_reg(tod_num, "TOD_WRITE_CMD", 0x1)

    def write_tod_relative(self, tod_num, tod_subns=0, tod_ns=0, tod_sec=0, add=True):
        data = []
        data += [tod_subns & 0xff]
        data += [byte for byte in tod_ns.to_bytes(4, byteorder='little')]
        data += [byte for byte in tod_sec.to_bytes(6, byteorder='little')]
        self.board.dpll.modules["TODWrite"].write_reg_mul(tod_num, "TOD_WRITE_SUBNS", 
                data)
        self.board.dpll.modules["TODWrite"].write_reg(tod_num, "TOD_WRITE_CMD", 0x0)
        if ( add ):
            print(f"Write TOD Add Relative tod{tod_num} -> {data}")
            # immediate delta TOD plus
            self.board.dpll.modules["TODWrite"].write_reg(tod_num, "TOD_WRITE_CMD", 0x11)
        else:
            print(f"Write TOD Minus Relative tod{tod_num} -> {data}")
            # immediate delta TOD minus
            self.board.dpll.modules["TODWrite"].write_reg(tod_num, "TOD_WRITE_CMD", 0x21)


    def adjust_tod_signed_nanoseconds(self, tod_num, nanosecond_val, include_decoder=False):
        print(f"Board {self.board.board_num} adjusting TOD{tod_num} nanosecondval={nanosecond_val}")
        if include_decoder:
            nanosecond_val -= (118 * ( 1 / 25e6 ) ) * 1e9
        add = False 
        if ( nanosecond_val < 0 ):
            nanosecond_val *= -1
            add = True


        tod_diff = nanoseconds_to_time(nanosecond_val)
        tod_subns = tod_diff[0]
        tod_ns = 0
        for i , byte in enumerate(tod_diff[1:5]):
            tod_ns += byte << (i * 8) 
        tod_sec = 0
        for i, byte in enumerate(tod_diff[5:]):
            tod_sec += byte << (i*8)

        self.board.dpof.write_tod_relative(tod_num, tod_subns, tod_ns,
                tod_sec, add)

    def adjust_tod(self, tod_num, local_tod, remote_tod, include_decoder=False):
        #print(f"Board {self.board_num} adjusting TOD{tod_num} to match far side")
        tod_diff, flag = time_difference_with_flag(local_tod, remote_tod, include_decoder)

        print(f"TOD_diff = {tod_diff}, flag = {flag}")
        tod_subns = tod_diff[0]
        tod_ns = 0
        for i , byte in enumerate(tod_diff[1:5]):
            tod_ns += byte << (i * 8) 
        tod_sec = 0
        for i, byte in enumerate(tod_diff[5:]):
            tod_sec += byte << (i*8)
        # tod_diff is only 9 bytes , ignoring top handshake bytes


        hex_diff = [hex(val) for val in tod_diff]
        if ( flag == 1 ):
            print(f"Board {self.board.board_num} Local TOD{tod_num} {local_tod} > remote {remote_tod}, shift {hex_diff}")
            #print(f"TOD {tod_num}, subns={tod_subns}, ns={tod_ns}, sec={tod_sec}")
            # flag = 1 , local_tod > remote_tod
            self.board.dpof.write_tod_relative(tod_num, tod_subns, tod_ns, tod_sec, False)

        elif ( flag == -1 ):
            print(f"Board {self.board.board_num} Local TOD{tod_num} {local_tod} < remote {remote_tod}, shift {hex_diff}")
            #print(f"TOD {tod_num}, subns={tod_subns}, ns={tod_ns}, sec={tod_sec}")
            # flag = -1 , local_tod < remote_tod
            self.board.dpof.write_tod_relative(tod_num, tod_subns, tod_ns, tod_sec, True)
        else:
            # flag = 0, equal , do nothing
            pass



    def add_to_average_tod_error(self, channel_num, local_tod, remote_tod, include_decoder=False):
        tod_diff = time_difference_signed_nanoseconds(local_tod, remote_tod, include_decoder)
        self.add_to_average_tod_error_ns(channel_num, tod_diff)
        
    def add_to_average_tod_error_ns(self, channel_num, nanosecond_val):
        self.average_tod_errors[channel_num].update(nanosecond_val)

    def get_average_tod_error(self, channel_num, clear_avg=False):
        return self.average_tod_errors[channel_num].get_average(clear_avg)

    def get_average_tod_count(self, channel_num):
        return self.average_tod_errors[channel_num].has_data()


    def inform_new_master(self, decoder_num):
        self.master_decoder = decoder_num
        self.following_master = True
        self.master_tod_differences = [] # for averaging

    #### Top level call, when following far side,
    # should call these after doing initial TOD adjustment to far side
    def start_follow_far_side(self, decoder_num):
        self.channels[decoder_num//2].set_follow_far_side()
        self.inform_new_master(decoder_num//2)

    def stop_follow_far_side(self):
        self.channels[self.master_decoder//2].stop_follow_far_side()
        self.master_decoder = -1
        self.following_master = False

    def get_channels_following(self):
        channels_following = []
        for index, chan in enumerate(self.channels):
            if ( chan.far_side_following ):
                channels_following.append(index)
        return channels_following

  


