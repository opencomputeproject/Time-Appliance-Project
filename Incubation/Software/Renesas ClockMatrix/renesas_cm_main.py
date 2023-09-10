from enum import Enum


class Chipset:
    def __init__(self, name, write_func, read_func):
        self.name = name
        self.write_func = write_func
        self.read_func = read_func
        self.gpios = []
        self.dpll_modules = []
        
        
    # helper function, assumes start_offset is [7:0] , and counts up
    # could optimize if put more i2c or spi details in here
    def write_multibyte_reg(self, base_addr, start_offset, value, num_bytes):
        for i in range(0,num_bytes):
            self.write_func(base_addr, start_offset + i, (value >> (8*i)) & 0xff)
    
    # useful for triggering, read/write same register with no modify
    def write_reg_nomodify(self, base_addr, offset):
        self.write_func(base_addr, offset, self.read_func(base_addr,offset))
    
    def add_gpio_module(self, gpio):
        # if using any gpio modules 
        if ( len(self.gpios) == 0 ):
            # init gpio level as 3.3
            self.write_func(0xc8c0, 0x0, 0x1)
        self.gpios.append(gpio)

    def add_dpll_module(self, dpll_module):
        self.dpll_modules.append(dpll_module)

    def initialize(self):
        # Perform chipset initialization here
        print(f"Initializing {self.name} chipset")
        
        
    def write_generic_i2c_bin_config(self, file=""):
        # an exported file from Timing Commander, Format=Generic, Filter=All, Protocol=i2c, AddressType=Two Byte addresses
        with open(file, 'r') as file:
            for line in file:
                if "Offset" in line:
                    parts = line.strip().split()
                    address = int(parts[3][0:4],16)
                    full_data = parts[5][2:]
                    data_bytes = []
                    for i in range(0, len(full_data), 2):
                        substr = full_data[i:i+2]
                        data_bytes.append(int(substr,16))
                        
                    print(f"Address 0x{address:X} = {full_data}, {data_bytes}")
                    for index, val in enumerate(data_bytes):
                        self.write_func(address+index, 0x0, val)
                        

              

    def shutdown(self):
        # Perform chipset shutdown here
        print(f"Shutting down {self.name} chipset")

    def __str__(self):
        return f"Chipset: {self.name}\nGPIOs: {len(self.gpios)}\nDPLL Modules: {len(self.dpll_modules)}"



#########
# General architecture of a module
# 1. it writes config directly to chipset
# 2. reads can be cached, but for now implementing by reading directly

class gpiomode(Enum):
    INPUT = 0
    OUTPUT = 1
class cm_gpio:
    def __init__(self, number: int, chipset):
        if number < 0 or number > 5:
            print(f"GPIO module init with invalid number {number}")
            self.number = -1
            return
        self.number = number
        self.chipset = chipset
        self.direction = ""
        self.value = -1
        self.base_addr = 0xc8c2 + (0x12 * number)

    # mode can be lots of values in theory
    # coding for 1 = output, 0 = input
    def configure_pin(self, mode: int, value: bool):
        # write gpio enable function and cmos 
        if ( mode == gpiomode.INPUT ):
            self.chipset.write_func(self.base_addr, 0x10, 0x0) # trigger register
        elif ( mode == gpiomode.OUTPUT ):            
            read_val = self.chipset.read_func(0xc160, 0x0)
            if ( value ):                
                read_val |= (1 << self.number)
            else:
                read_val &= ~(1 << self.number)
            self.chipset.write_func(0xc160, 0x0, read_val)
            self.chipset.write_func(self.base_addr, 0x10, 0x4) # trigger register

    # returns [mode, value]
    def read_pin_mode(self) -> [int, int]:
        mode = self.chipset.read_func(self.base_addr, 0x10)
        if ( mode & 0x4 ) and ~( mode & 0x1 ):
            mode = gpiomode.OUTPUT
        elif ~(mode & 0x4) and ~(mode & 0x1):
            mode = gpiomode.INPUT
            
        val = self.chipset.read_func(0xc160, 0x0)
        val = (val >> self.number) & 0x1
        return [ mode, val ]  

    

    def __str__(self):
        return f"GPIO Module: {self.number}"
        
        
 



#####################
# Inputs 0-7 are the positive pins in single ended mode, or the differential signal
# Inputs 8-15 are the negative pins only in single ended mode, differential mode not valid
# including reference monitor configuration as part of this clsas
class cm_input:
    base_addrs = [0xc1b0, 0xc1c0, 0xc1d0, 0xc200,
        0xc210, 0xc220, 0xc230, 0xc240, 0xc250,
        0xc260, 0xc280, 0xc290, 0xc2a0, 0xc2b0,
        0xc2c0, 0xc2d0]
    ref_mon_base_addrs = [ 0xc2e0, 0xc2ec, 0xc300,
        0xc30c, 0xc318, 0xc324, 0xc330, 0xc33c, 0xc348,
        0xc354, 0xc360, 0xc36c, 0xc380, 0xc38c, 0xc398,
        0xc3a4 ]
    def __init__(self, number: int, chipset):
        if number < 0 or number > 15:
            print(f"DPLL input init with invalid number {number}")
            self.number = -1
            return
        self.chipset = chipset
        self.number = number
        self.base_addr = self.base_addrs[number]
        self.ref_mon_base_addr = self.ref_mon_base_addrs[number]
    def is_valid(self) -> bool:
        if self.num >= 8:
            pos_mode = self.base_addrs[self.number-8]
            pos_val = self.chipset.read_func(pos_mode, 0xd)
            if ( pos_val & (1<<5) ): # bit 5, 0 means single ended, 1 means differential
                return False           
        return True        
        
    def is_enabled(self) -> bool:
        val = self.chipset.read_func(self.base_addr, 0xd)
        return (val & 1)
        
    def is_single_ended(self) -> bool:
        # must be valid and enabled 
        if self.is_valid() and self.is_enabled():
            # if it's valid and enabled, and above 8, then must be single ended
            if self.num >= 8:
                return True
            else:            
                val = self.chipset.read_func(self.base_addr, 0xd)
                return ( val & (1<<5) )                
        return False
        
        
    # chatgpt algorithm
    def calculate_M_N(self,frequency):
        if 0.5 <= frequency <= 1e9:  # Check if the frequency is within the valid range
            max_M = (1 << 48) - 1
            max_N = (1 << 16) - 1
            best_error = float('inf')
            result_M, result_N = 0, 1

            for N in range(1, max_N + 1):
                M = int(frequency * N)

                if 0 < M <= max_M:
                    calculated_frequency = M / N
                    error = abs(calculated_frequency - frequency)

                    if error < best_error:
                        best_error = error
                        result_M, result_N = M, N

            return result_M, result_N
        else:
            raise ValueError("Frequency is out of the valid range (0.5 Hz to 1 GHz).")

            
        
    # differential mode, max is 1GHz, in single ended, max is 325MHz
    def set_input_frequency(self, freq: float=25e6) -> bool:
        # Input frequency as M/N, N = unsigned 16-bit, M = unsigned 48-bit
        if ( freq < 0.5 ):
            print(f"Input {number} set frequency invalid too low {freq}")
            return False
            
        if ( self.is_single_ended() ):
            # 325MHz
            if ( freq > 325e6 ):
                print(f"Input {number} set frequency invalid {freq}")
                return False
        else:
            # 1GHz max
            if ( freq > 1e9 ):
                print(f"Input {number} set frequency invalid {freq}")
                return False

        m, n = self.calculate_M_N(freq)
        
        # write the registers
        self.chipset.write_multibyte_reg(self.base_addr, 0x0, m, 6)
        self.chipset.write_multibyte_reg(self.base_addr, 0x6, n, 2)
        
        if freq > 150e6:
            # need to setup input divider 
            # not planning to use for now, leave for later
            print(f"DPLL {self.number} set freq needs input divider! freq {freq}")  

        self.chipset.write_reg_nomodify(self.base_addr, 0xd)
            
        return True

        
    # signed 16-bit phase offset value in ITDC_UIs
    def set_inphase_offset(self, val: int=0) -> bool:
        val &= 0xffff 
        self.chipset.write_multibyte_reg(self.base_addr, 0xa, val, 2)
        self.chipset.write_reg_nomodify(self.base_addr, 0xd)
        return True
        
    # specify another input to be used as the frame pulse or sync pulse for the current input
    # source = 0x0 -> 0xf , CLK0 -> CLK15 ; 0x10 -> 0x1f -> PPS from PWM Decoder 0 -> 15    
    def setup_sync_pulse(self, enable: bool=False, neg_edge: bool=False, resample_enable: bool=True, 
        source: int=0) -> bool:
        val = 0
        val |= (enable << 7)
        val |= (neg_edge << 6)
        val |= (resample_enable << 5)
        val += source & 0x1f
        self.chipset.write_func(self.base_addr, 0xc, val)
        self.chipset.write_reg_nomodify(self.base_addr, 0xd)
        return True

    # is_differential only applicable for clk0-7
    def setup_input_mode(self, is_differential: bool=False, diff_mode_pmos: bool=False, 
        invert_input: bool=False, input_enabled: bool=True) -> bool:
        val = 0
        val |= input_enabled
        val |= (invert_input << 3)
        val |= (diff_mode_pmos << 4)
        val |= (is_differential << 5)
        self.chipset.write_func(self.base_addr, 0xd, val)
        
        return True
    
    ##### Reference monitor functions  
    
    def setup_reference_monitor(self, enable: bool=False, use_nondiv_clock: bool=True) -> bool:
        val = self.chipset.read_func(self.ref_mon_base_addr, 0xb)
        if ( enable ):
            val |= 1
        else:
            val &= 0xfe
        if ( use_nondiv_clock ):
            val |= 1<<5
        else:
            val &= 0xDF
            
        self.chipset.write_func(self.ref_mon_base_addr, 0xb, val)
        return True
        
    # valid_interval_seconds is 4-bit value, 0 means use short interval
    # valid_interval_short is 8 bit value, 0 means 5 seconds, otherwise in units of 10 milliseconds
    # freq_offset_limit is an 3-bit value, but enumerated. A means good I think, R means bad
    #       0 = 9.2ppm(A), 12ppm(R)
    #       1 = 13.8ppm(A), 18ppm(R)
    #       2 = 24.6ppm(A), 32ppm(R)
    #       3 = 36.6ppm(A), 47.5ppm(R)
    #       4 = 40ppm(A), 52ppm(R)
    #       5 = 52ppm(A), 67.5ppm(R)
    #       6 = 64ppm(A), 83ppm(R)
    #       7 = 100ppm(A), 130ppm(R)
    def setup_refmon_frequency_offset_monitor(self, enable: bool=False,
        valid_interval_seconds: int=0,
        valid_interval_short: int=0,
        frequency_offset_limit: int=0) -> bool:     
        
        val = ( valid_interval_seconds & 0xf ) << 3
        val += frequency_offset_limit & 0x7
        self.chipset.write_func(self.ref_mon_base_addr, 0x0, val)
        self.chipset.write_func(self.ref_mon_base_addr, 0x1, valid_interval_short & 0xff)
        #handle enable
        val = self.chipset.read_func(self.ref_mon_base_addr, 0xb)
        if (enable):
            val |= 1<<2
        else:
            val &= 0xFB
            
        self.chipset.write_func(self.ref_mon_base_addr, 0xb, val)
        return True
        

    # los_tolerance = unsigned 16-bit in milliseconds, 0 means trigger immediately upon LOS detect
    # los_gap = 2 bits num consecutive missing edges to declare LOS
    #       0=disabled, 1=1 edges, 2=2 edges, 3=5 edges
    # los_margin , detection margin, 0 = tight 1% freq error, 1 = loose 25% freq error
    def setup_refmon_loss_of_signal_monitor(self, enable: bool=False,
        los_tolerance: int=0, 
        los_gap: int=0,
        los_margin: bool=0) -> bool:
        
        los_tolerance &= 0xffff
        self.chipset.write_multibyte_reg(self.ref_mon_base_addr, 0x8, los_tolerance, 2)
        
        val = ((los_gap & 0x3) << 1) + los_margin
        self.chipset.write_func(self.ref_mon_base_addr, 0xa, val)
        
        #handle enable
        val = self.chipset.read_func(self.ref_mon_base_addr, 0xb)
        if (enable):
            val |= 0x2
        else:
            val &= 0xfd            
        self.chipset.write_func(self.ref_mon_base_addr, 0xb, val)    
        return True
    
    # disqual_timer = 2-bit, 0 = 2.5s, 1 = 1.25ms, 2 = 25ms, 3 = 50ms
    # qual_timer, multiplier of disqual timer to qualify input 
    #       0 = 4x , 1 = 2x, 2 = 8x, 3 = 16x
    # activity_limit , 3 bit enum
    #       0 = 1000ppm, 1 = 260ppm, 2 = 130ppm, 3 = 83ppm, 
    #       4 = 65ppm, 5 = 52ppm, 6 = 18ppm, 7 = 12ppm
    def setup_refmon_nonactivity_monitor(self, enable: bool=False,
        disqual_timer: int=0,
        qual_timer: int=0,
        activity_limit: int=0) -> bool:
        val = ((disqual_timer & 0x3) << 3)
        val += ((qual_timer & 0x3) << 5)
        val += (activity_limit & 0x7)
        self.chipset.write_func(self.ref_mon_base_addr, 0x6, val)
        
        #handle enable
        val = self.chipset.read_func(self.ref_mon_base_addr, 0xb)
        if (enable):
            val |= (1<<3)
        else:
            val &= 0xf7           
        self.chipset.write_func(self.ref_mon_base_addr, 0xb, val)    
        return True
    

    # phase_threshold is 16-bit value in nanoseconds
    # phase_transient is 16-bit phase transient detection period in units of 100uS
    def setup_refmon_phase_transient_cfg(self, enable: bool=False,
        phase_threshold: int=0,
        phase_transient: int=0) -> bool:
        self.chipset.write_multibyte_reg(self.ref_mon_base_addr, 0x2, phase_threshold&0xffff, 2)
        self.chipset.write_multibyte_reg(self.ref_mon_base_addr, 0x4, phase_transient&0xffff, 2)
        
        #handle enable
        val = self.chipset.read_func(self.ref_mon_base_addr, 0xb)
        if (enable):
            val |= (1<<4)
        else:
            val &= 0xEF          
        self.chipset.write_func(self.ref_mon_base_addr, 0xb, val)    
        return True
        
        
        
class cm_output:
    base_addrs=[ 0xca14, 0xca24, 0xca34, 0xca44,
        0xca54, 0xca64, 0xca80, 0xca90, 0xcaa0,
        0xcab0, 0xcac0, 0xcad0 ]
    def __init__(self, number: int, chipset):
        if number < 0 or number > 11:
            print(f"Output module init with invalid number {number}")
            self.number = -1
            return
        self.number = number
        self.chipset = chipset
        self.base_addr = self.base_addrs[number]
        
    # div is 32 bit value, output clock is FOD (DCO) frequency divided by this 
    def configure_output_divider(self, div: int) -> bool:
        self.chipset.write_multibyte_reg(self.base_addr, 0x0, div&0xffffffff, 4)
        return True
          
    # dc_high is high pulse width, duty cycle = dc_high / (out divider - dc _high)
    # 0 = 50/50
    def configure_duty_cycle(self, dc_high: int=0) -> bool:
        self.chipset.write_multibyte_reg(self.base_addr, 0x4, dc_high&0xffffffff, 4)
        return True
        
    # common mode enum, voltage = 0.9V + (0.2 * value), 3 bits
    # vswing, single ended voltage swing, 0 = 410mV, 1 = 600mV, 2 = 750mV, 3 = 900mV , 2 bits
    # pad_mode, 0 = high-Z, 1 = differential, 2 = LVCMOS inverted, 3 = LVCMOS in-phase
    #       6 = LVCMOS, Q enabled, nQ High-Z
    # phase sync is just enabling output phase sync
    # squelch high is what value to drive when squelching
    # enable squelch, squelch output
    # vddo_level, 0 = 1.8V, 1 = 3.3V, 2 = 2.5V, 3 = 1.5V, 4 = 1.2V
    # output impedance complex, listing 3.3V value heres
    #       0 = 38 ohm, 1 = 25 ohm, 2 = 18 ohm, 3 = 15 ohm
    def configure_output_voltage(self, common_mode: int=2,
        vswing: int=0, pad_mode: int=1,
        phase_sync_disable: bool=0, squelch_high: bool=0,
        enable_squelch: bool=0, vddo_level: int=1,
        output_impedance: int=0) -> bool:
        
        val = (common_mode & 0x7) << 5
        val += (vswing & 3) << 3
        val += (pad_mode & 0x7)
        self.chipset.write_func(self.base_addr, 0x8, val)
        
        val = output_impedance & 0x3
        val += (vddo_level & 0x7) << 2
        val += ( not enable_squelch << 5)
        val += squelch_high << 6
        val += phase_sync_disable << 7
        self.chipset.write_func(self.base_addr, 0x9, val)
        return True
    

    # Signed 32-bit value in FOD (DCO) cycles to apply a phase shift to output clock
    def set_output_phase_adj(self, value: int=0) -> bool:
        self.chipset.write_multibyte_reg(self.base_addr, 0xc, value&0xffffffff)
        return True
        
 

#######################################
# PWM Encoding and decoding
# not coding for signature mode, I want to use max capability of PWM

class cm_pwm_encoder: 
    base_addrs=[ 0xcb00, 0xcb08, 0xcb10, 0xcb18 ]

    def __init__(self, number: int, chipset):
        if number < 0 or number > 3:
            print(f"PWM Encoder module init with invalid number {number}")
            self.number = -1
            return
        self.number = number
        self.chipset = chipset
        self.base_addr = self.base_addrs[number]

    # encoder_id = unique PWM encoder ID in the PWM network
    # output_index = 0 for primary output ( odd output of DPLL ), 1 for secondary ( even output of DPLL )
    # tod_index = 0-3 , DPLL index from which TOD PPS is transmitted 
    def config_pwm_encoder(self, enable: bool=False,
        encoder_id: int=0,
        output_index: bool=0,
        tod_index: int=0 ) -> bool:
        val = (tod_index & 0x3) + (output_index << 2)
        self.chipset.write_func(self.base_addr, 0x1, val)
        val = 0x4 + enable # always enable TOD transmission 
        self.chipset.write_func(self.base_addr, 0x4, val)


class cm_pwm_decoder:
    base_addrs=[ 0xcb40, 0xcb48, 0xcb50, 0xcb58,
        0xcb60, 0xcb68, 0xcb70, 0xcb80, 0xcb88,
        0xcb90, 0xcb98, 0xcba0, 0xcba8, 0xcbb0, 
        0xcbb8, 0xcbc0 ]
        
    def __init__(self, number: int, chipset):
        if number < 0 or number > 15:
            print(f"PWM decoder module init with invalid number {number}")
            self.number = -1
            return
        self.number = number
        self.chipset = chipset
        self.base_addr = self.base_addrs[number]
        
    # pwm_pps_rate = 15-bit value, PWM PPS rate in units of 0.5Hz, presumably how many frames to send per second
    def config_pwm_decoder(self, enable: bool=False,
        decoder_id: int=0,
        generate_pps: bool=True,
        pwm_pps_rate: int=2,
        put_tod_frame_in_fifo: bool=False ) -> bool:
        
        self.chipset.write_func(self.base_addr, 0x0, pwm_pps_rate&0xff)
        
        val = (generate_pps << 7) + ((pwm_pps_rate >> 8) & 0x7f)
        self.chipset.write_func(self.base_addr, 0x1, val)
        
        self.chipset.write_func(self.base_addr, 0x2, decoder_id & 0xff)
        
        val = 0
        val += enable
        val += (put_tod_frame_in_fifo << 2)
        self.chipset.write_func(self.base_addr, 0x5, val)
        


class cm_pwm_user_data: 
    def __init__(self, number: int, chipset):
        if number < 0 or number > 0:
            print(f"PWM user data module init with invalid number {number}")
            self.number = -1
            return
        self.number = number
        self.chipset = chipset
        self.base_addr = 0xcbc8
        
    # encoder id is for transmitter PWM ID
    # decoder id is the receiver PWM ID, 0xff is for broadcast
    # setup these encoder / decoder IDs associated with PWM channels before using this
    def set_pwm_user_ids(self, encoder_id: int=0,
        dst_decoder_id: int=0 ):
        self.chipset.write_func(self.base_addr, 0x0, encoder_id & 0xff)
        self.chipset.write_func(self.base_addr, 0x1, decoder_id & 0xff)     

    def start_receive_pwm_user_data(self) -> bool:        
        self.chipset.write_func(self.base_addr, 0x2, 0x0) # clear byte count
        self.chipset.write_func(self.base_addr, 0x3, 0x0) # set to idle to receive
        return True
        
    def read_receive_pwm_user_data(self) -> [ bool, int, [] ]:
        val = self.chipset.read_func(self.base_addr, 0x3)
        if ( val == 0xb ): #reception successful
            # read the byte count
            val = self.chipset.read_func(self.base_addr, 0x2)
            read_buffer = []
            for i in range(val):
                read_buffer.append( self.chipset.read_func(0xcf80, i))
            return [True, val, read_buffer]
        else:
            return [False, 0, []]

    # use set_pwm_user_ids first based on who you want to talk to , and setup PWM IDs 
    def transmit_pwm_user_data(self, data: [int], data_count: int=0) -> bool:
        # write byte count
        self.chipset.write_func(self.base_addr, 0x2, byte_count & 0xff)
        self.chipset.write_func(self.base_addr, 0x3, 0x1) # transmission request
        count = 0
        while count < 100:
            val = self.chipset.read_func( self.base_addr, 0x3 )
            if ( val == 0x3 ): # transmission acknowledgement
                count = 0
                break
            count += 1
        if ( count >= 100 ): # timed out for some reason
            print("Transmit pwm user data failed, no acknowledge")
            return False
        # write the data to byte buffer 
        self.chipset.write_multibyte_reg( 0xcf80, 0x0, data, data_count )
        
        self.chipset.write_func(self.base_addr, 0x3, 0x2) # start transmission
        
        count = 0
        while count < 100:
            val = self.chipset.read_func(self.base_addr, 0x3)
            if ( val == 0x5 ): # successful transmission
                count = 0
                break
            count += 1
        if ( count >= 100 ): 
            print("Transmit pwm user data didn't get successful transmission!")
            return False
        return True
        





class cm_tod:
    base_addrs=[ 0xcbcc, 0xcbce, 0xcbd0, 0xcbd2 ]
    write_base_addrs=[ 0xcc00, 0xcc10, 0xcc20, 0xcc30 ]
    read_primary_base_addrs= [0xcc40, 0xcc50, 0xcc60, 0xcc80 ]
    read_secondary_base_addrs= [ 0xcc90, 0xcca0, 0xccb0, 0xccc0]

    def __init__(self, number: int, chipset):
        if number < 0 or number > 3:
            print(f"TOD module init with invalid number {number}")
            self.number = -1
            return
        self.number = number
        self.chipset = chipset
        self.base_addr = self.base_addrs[number]
        self.write_base_addr = self.write_base_addrs[number]
        self.read_primary_base_addr = self.read_primary_base_addrs[number]
        self.read_secondary_base_addr = self.read_secondary_base_addrs[number]
        
    # even_pps mode generates a pulse every 2 seconds instead of 1 seconds
    # tod_write_trigger enums
    #       0 = disabled, 1 = immediate, 2 = selected reference clock input
    #       3 = selected PWM decoders 1PPS output, 4 = rising edge of TOD PPS signal
    #       5 = feedback from FOD, 6 = selected gpio (Im not using GPIO for this)
    # tod_write_type = 0 for absolute, 1 for delta TOD plus, 2 for delta TOD minus
    # pwm_decoder_index = decoder index to use for a trigger, when write trigger = 3
    # input_ref_index = input reference index to use as trigger, when write trigger = 2
    def config_tod(self, enable: bool=False,
        disable_output_channel_sync: bool=False,
        enable_even_pps: bool=False,
        tod_write_trigger: int=1,
        tod_write_type: int=0,
        pwm_decoder_index: int=0,
        input_ref_index: int=0
        ) -> bool:
        
        val = ( enable_even_pps << 2 ) 
        val += disable_output_channel_sync << 1
        val += enable
        self.chipset.write_func(self.base_addr, 0x0, val)
        
        val = (pwm_decoder_index & 0xf) << 4
        val += (input_ref_index & 0xf)
        self.chipset.write_func(self.write_base_addr, 0xd, val)
        
        val = tod_write_trigger & 0xf
        val += (tod_write_type & 0x3) << 4
        self.chipset.write_func(self.write_base_addr, 0xf, val)     
        
        return True
          
    # 8-bit sub_nanosecond in units of 1/256 nanoseconds 
    # 4-byte nanoseconds, max value = 999,999,999
    # 6-byte seconds,     
    def write_tod_value(sub_nanosecond: int=0,
        nanoseconds: int=0,
        seconds: int=0) -> bool:   
        self.chipset.write_func(self.write_base_addr, 0x0, sub_nanosecond & 0xff)
        self.chipset.write_multibyte_reg(self.write_base_addr, 0x1, nanoseconds & 0xffffffff, 4)
        self.chipset.write_multibyte_reg(self.write_base_addr, 0x5, seconds & 0xffffffffffff, 6)
        self.chipset.write_reg_nomodify(self.write_base_addr, 0xf) # trigger register
        return True
        
        
    ### Read TOD stuff left for later, it provides two timestamp inputs effectively
    
    
    
    
    
    

###################################################################
#### DPLL 0 - 3 settings
#### for now, just do this from GUI 
class cm_dpll:
    def __init__(self, number: int, chipset):
        # designing for 8A34002, 4 DPLLs
        if number < 0 or number > 3:
            print(f"DPLL module init with invalid number {number}")
            self.number = -1
            return
        if number == 0:
            self.base_addr = 0xc3b0
            self.ctrl_addr = 0xc600
            self.write_phase = 0xc818
            self.write_freq = 0xc838
            self.phase_pullin = 0xc880
        elif number == 1:
            self.base_addr = 0xc400
            self.ctrl_addr = 0xc63c
            self.write_phase = 0xc81c
            self.write_freq = 0xc840
            self.phase_pullin = 0xc888
        elif number == 2:
            self.base_addr = 0xc438
            self.ctrl_addr = 0xc680
            self.write_phase = 0xc820
            self.write_freq = 0xc848
            self.phase_pullin = 0xc890          
        elif number == 3:
            self.base_addr = 0xc480
            self.ctrl_addr = 0xc6bc
            self.write_phase = 0xc824
            self.write_freq = 0xc850
            self.phase_pullin = 0xc898
            
        self.number = number
        self.chipset = chipset
        
        
    # pass how many sources 
    def set_lock_priority(self, sources: [int]):
        pass

    def __str__(self):
        return f"DPLL Module: {self.number}"
        




def dpll_write_func( base_addr: int=0, offset: int=0,
    value: int=0) -> bool:
    print(f"DPLL Write Base 0x{base_addr:X} offset 0x{offset:X} = 0x{value:X}")
    return True
    
def dpll_read_func(base_addr: int=0, offset: int=0) -> int:
    print(f"DPLL Read Base 0x{base_addr:X} offset 0x{offset:X}")
    return 0

# Example usage
if __name__ == "__main__":
    chipset = Chipset("Example Chipset", dpll_write_func, dpll_read_func)

    chipset.write_generic_i2c_bin_config("8A34012_20230901_123654_CLK3_PWMpos_SlaveCard_delaycomp_export.txt")
    chipset.initialize()
    print(chipset)


