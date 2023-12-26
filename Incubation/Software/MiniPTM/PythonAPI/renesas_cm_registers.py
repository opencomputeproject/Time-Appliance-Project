import re
from renesas_cm_gpio import cm_gpios

class BitField:
    def __init__(self, start_bit, length):
        self.start_bit = start_bit
        self.length = length
        self.mask = (1 << length) - 1

    def get_value(self, reg_value):
        """ Extract the value of this bit field from the register value. """
        return (reg_value >> self.start_bit) & self.mask

    def set_value(self, reg_value, field_value):
        """ Set the value of this bit field in the register value. """
        field_value &= self.mask  # Ensure the field value fits in the bit field
        reg_value &= ~(self.mask << self.start_bit)  # Clear the bit field area
        reg_value |= (field_value << self.start_bit)  # Set the new value
        return reg_value


# PWM_ENCODER layout structure
PWM_ENCODER_LAYOUT = {
    "PWM_ENCODER_ID": {"offset": 0x000, "fields": {"ENCODER_ID": BitField(0, 8)}},
    "PWM_ENCODER_CNFG": {"offset": 0x001, "fields": {"PPS_SEL": BitField(3, 1), "SECONDARY_OUTPUT": BitField(2, 1), "TOD_SEL": BitField(0, 2)}},
    "PWM_ENCODER_SIGNATURE_0": {"offset": 0x002, "fields": {"FIFTH_SYMBOL": BitField(6, 2), "SIXTH_SYMBOL": BitField(4, 2), "SEVENTH_SYMBOL": BitField(2, 2), "EIGHTH_SYMBOL": BitField(0, 2)}},
    "PWM_ENCODER_SIGNATURE_1": {"offset": 0x003, "fields": {"FIRST_SYMBOL": BitField(6, 1), "SECOND_SYMBOL": BitField(4, 2), "THIRD_SYMBOL": BitField(2, 2), "FOURTH_SYMBOL": BitField(0, 2)}},
    "PWM_ENCODER_CMD": {"offset": 0x004, "fields": {"TOD_AUTO_UPDATE": BitField(3, 1), "TOD_TX": BitField(2, 1), "SIGNATURE_MODE": BitField(1, 1), "ENABLE": BitField(0, 1)}}
}

# PWM_DECODER layout structure
PWM_DECODER_LAYOUT = {
    "PWM_DECODER_CNFG": {"offset": 0x000, "fields": {"PPS_RATE_0_7": BitField(0, 8)}},
    "PWM_DECODER_CNFG_1": {"offset": 0x001, "fields": {"GENERATE_PPS": BitField(7, 1), "PPS_RATE_8_14": BitField(0, 7)}},
    "PWM_DECODER_ID": {"offset": 0x002, "fields": {"DECODER_ID": BitField(0, 8)}},
    "PWM_DECODER_SIGNATURE_0": {"offset": 0x003, "fields": {"FIFTH_SYMBOL": BitField(6, 2), "SIXTH_SYMBOL": BitField(4, 2), "SEVENTH_SYMBOL": BitField(2, 2), "EIGHTH_SYMBOL": BitField(0, 2)}},
    "PWM_DECODER_SIGNATURE_1": {"offset": 0x004, "fields": {"FIRST_SYMBOL": BitField(6, 1), "SECOND_SYMBOL": BitField(4, 2), "THIRD_SYMBOL": BitField(2, 2), "FOURTH_SYMBOL": BitField(0, 2)}},
    "PWM_DECODER_CMD": {"offset": 0x005, "fields": {"TOD_FRAME_ACCESS_EN": BitField(2, 1), "SIGNATURE_MODE": BitField(1, 1), "ENABLE": BitField(0, 1)}}
}

# TOD layout structure
TOD_LAYOUT = {
    "TOD_CFG": {"offset": 0x000, "fields": {"TOD_EVEN_PPS_MODE": BitField(2, 1), "TOD_OUT_SYNC_DISABLE": BitField(1, 1), "TOD_ENABLE": BitField(0, 1)}}
}

# TOD_WRITE layout structure
TOD_WRITE_LAYOUT = {
    "TOD_WRITE_SUBNS": {"offset": 0x000, "fields": {"SUBNS": BitField(0, 8)}},
    "TOD_WRITE_NS_0_7": {"offset": 0x001, "fields": {"NS_0_7": BitField(0, 8)}},
    "TOD_WRITE_NS_8_15": {"offset": 0x002, "fields": {"NS_8_15": BitField(0, 8)}},
    "TOD_WRITE_NS_16_23": {"offset": 0x003, "fields": {"NS_16_23": BitField(0, 8)}},
    "TOD_WRITE_NS_24_31": {"offset": 0x004, "fields": {"NS_24_31": BitField(0, 8)}},
    "TOD_WRITE_SECONDS_0_7": {"offset": 0x005, "fields": {"SECONDS_0_7": BitField(0, 8)}},
    "TOD_WRITE_SECONDS_8_15": {"offset": 0x006, "fields": {"SECONDS_8_15": BitField(0, 8)}},
    "TOD_WRITE_SECONDS_16_23": {"offset": 0x007, "fields": {"SECONDS_16_23": BitField(0, 8)}},
    "TOD_WRITE_SECONDS_24_31": {"offset": 0x008, "fields": {"SECONDS_24_31": BitField(0, 8)}},
    "TOD_WRITE_SECONDS_32_39": {"offset": 0x009, "fields": {"SECONDS_32_39": BitField(0, 8)}},
    "TOD_WRITE_SECONDS_40_47": {"offset": 0x00A, "fields": {"SECONDS_40_47": BitField(0, 8)}},
    "TOD_WRITE_RESERVED_0": {"offset": 0x00B, "fields": {"RESERVED": BitField(0, 8)}},
    "TOD_WRITE_COUNTER": {"offset": 0x00C, "fields": {"WRITE_COUNTER": BitField(0, 8)}},
    "TOD_WRITE_SELECT_CFG_0": {"offset": 0x00D, "fields": {"PWM_DECODER_INDEX": BitField(4, 4), "REF_INDEX": BitField(0, 4)}},
    "TOD_WRITE_RESERVED_1": {"offset": 0x00E, "fields": {"RESERVED": BitField(0, 8)}},
    "TOD_WRITE_CMD": {"offset": 0x00F, "fields": {"TOD_WRITE_TYPE": BitField(4, 2), "TOD_WRITE_SELECTION": BitField(0, 4)}}
}


# TOD_READ_PRIMARY layout structure
TOD_READ_PRIMARY_LAYOUT = {
    "TOD_READ_PRIMARY_SUBNS": {"offset": 0x000, "fields": {"SUBNS": BitField(0, 8)}},
    "TOD_READ_PRIMARY_NS_0_7": {"offset": 0x001, "fields": {"NS_0_7": BitField(0, 8)}},
    "TOD_READ_PRIMARY_NS_8_15": {"offset": 0x002, "fields": {"NS_8_15": BitField(0, 8)}},
    "TOD_READ_PRIMARY_NS_16_23": {"offset": 0x003, "fields": {"NS_16_23": BitField(0, 8)}},
    "TOD_READ_PRIMARY_NS_24_31": {"offset": 0x004, "fields": {"NS_24_31": BitField(0, 8)}},
    "TOD_READ_PRIMARY_SECONDS_0_7": {"offset": 0x005, "fields": {"SECONDS_0_7": BitField(0, 8)}},
    "TOD_READ_PRIMARY_SECONDS_8_15": {"offset": 0x006, "fields": {"SECONDS_8_15": BitField(0, 8)}},
    "TOD_READ_PRIMARY_SECONDS_16_23": {"offset": 0x007, "fields": {"SECONDS_16_23": BitField(0, 8)}},
    "TOD_READ_PRIMARY_SECONDS_24_31": {"offset": 0x008, "fields": {"SECONDS_24_31": BitField(0, 8)}},
    "TOD_READ_PRIMARY_SECONDS_32_39": {"offset": 0x009, "fields": {"SECONDS_32_39": BitField(0, 8)}},
    "TOD_READ_PRIMARY_SECONDS_40_47": {"offset": 0x00A, "fields": {"SECONDS_40_47": BitField(0, 8)}},
    "TOD_READ_PRIMARY_COUNTER": {"offset": 0x00B, "fields": {"READ_COUNTER": BitField(0, 8)}},
    "TOD_READ_PRIMARY_SEL_CFG_0": {"offset": 0x00C, "fields": {"PWM_DECODER_INDEX": BitField(4, 4), "REF_INDEX": BitField(0, 4)}},
    "TOD_READ_PRIMARY_SEL_CFG_1": {"offset": 0x00D, "fields": {"DPLL_INDEX": BitField(0, 3)}},
    "TOD_READ_PRIMARY_CMD": {"offset": 0x00E, "fields": {"TOD_READ_TRIGGER_MODE": BitField(4, 1), "TOD_READ_TRIGGER": BitField(0, 4)}}
}

# TOD_READ_SECONDARY layout structure
TOD_READ_SECONDARY_LAYOUT = {
    "TOD_READ_SECONDARY_SUBNS": {"offset": 0x000, "fields": {"SUBNS": BitField(0, 8)}},
    "TOD_READ_SECONDARY_NS_0_7": {"offset": 0x001, "fields": {"NS_0_7": BitField(0, 8)}},
    "TOD_READ_SECONDARY_NS_8_15": {"offset": 0x002, "fields": {"NS_8_15": BitField(0, 8)}},
    "TOD_READ_SECONDARY_NS_16_23": {"offset": 0x003, "fields": {"NS_16_23": BitField(0, 8)}},
    "TOD_READ_SECONDARY_NS_24_31": {"offset": 0x004, "fields": {"NS_24_31": BitField(0, 8)}},
    "TOD_READ_SECONDARY_SECONDS_0_7": {"offset": 0x005, "fields": {"SECONDS_0_7": BitField(0, 8)}},
    "TOD_READ_SECONDARY_SECONDS_8_15": {"offset": 0x006, "fields": {"SECONDS_8_15": BitField(0, 8)}},
    "TOD_READ_SECONDARY_SECONDS_16_23": {"offset": 0x007, "fields": {"SECONDS_16_23": BitField(0, 8)}},
    "TOD_READ_SECONDARY_SECONDS_24_31": {"offset": 0x008, "fields": {"SECONDS_24_31": BitField(0, 8)}},
    "TOD_READ_SECONDARY_SECONDS_32_39": {"offset": 0x009, "fields": {"SECONDS_32_39": BitField(0, 8)}},
    "TOD_READ_SECONDARY_SECONDS_40_47": {"offset": 0x00A, "fields": {"SECONDS_40_47": BitField(0, 8)}},
    "TOD_READ_SECONDARY_COUNTER": {"offset": 0x00B, "fields": {"READ_COUNTER": BitField(0, 8)}},
    "TOD_READ_SECONDARY_SEL_CFG_0": {"offset": 0x00C, "fields": {"PWM_DECODER_INDEX": BitField(4, 4), "REF_INDEX": BitField(0, 4)}},
    "TOD_READ_SECONDARY_SEL_CFG_1": {"offset": 0x00D, "fields": {"DPLL_INDEX": BitField(0, 3)}},
    "TOD_READ_SECONDARY_CMD": {"offset": 0x00E, "fields": {"TOD_READ_TRIGGER_MODE": BitField(4, 1), "TOD_READ_TRIGGER": BitField(0, 4)}}
}

# INPUT layout structure
INPUT_LAYOUT = {
    "INPUT_IN_FREQ_M_0_7": {"offset": 0x000, "fields": {"M_0_7": BitField(0, 8)}},
    "INPUT_IN_FREQ_M_8_15": {"offset": 0x001, "fields": {"M_8_15": BitField(0, 8)}},
    "INPUT_IN_FREQ_M_16_23": {"offset": 0x002, "fields": {"M_16_23": BitField(0, 8)}},
    "INPUT_IN_FREQ_M_24_31": {"offset": 0x003, "fields": {"M_24_31": BitField(0, 8)}},
    "INPUT_IN_FREQ_M_32_39": {"offset": 0x004, "fields": {"M_32_39": BitField(0, 8)}},
    "INPUT_IN_FREQ_M_40_47": {"offset": 0x005, "fields": {"M_40_47": BitField(0, 8)}},
    "INPUT_IN_FREQ_N_0_7": {"offset": 0x006, "fields": {"N_0_7": BitField(0, 8)}},
    "INPUT_IN_FREQ_N_8_15": {"offset": 0x007, "fields": {"N_8_15": BitField(0, 8)}},
    "INPUT_IN_DIV_0_7": {"offset": 0x008, "fields": {"IN_DIV_0_7": BitField(0, 8)}},
    "INPUT_IN_DIV_8_15": {"offset": 0x009, "fields": {"IN_DIV_8_15": BitField(0, 8)}},
    "INPUT_IN_PHASE_0_7": {"offset": 0x00A, "fields": {"IN_PHASE_0_7": BitField(0, 8)}},
    "INPUT_IN_PHASE_8_15": {"offset": 0x00B, "fields": {"IN_PHASE_8_15": BitField(0, 8)}},
    "INPUT_IN_SYNC": {"offset": 0x00C, "fields": {"FRAME_SYNC_PULSE_EN": BitField(7, 1), "FRAME_SYNC_RESAMPLE_EDGE": BitField(6, 1), "FRAME_SYNC_RESAMPLE_EN": BitField(5, 1), "FRAME_SYNC_PULSE": BitField(0, 5)}},
    "INPUT_IN_MODE": {"offset": 0x00D, "fields": {"DPLL_PRED": BitField(7, 1), "MUX_GPIO_IN": BitField(6, 1), "IN_DIFF": BitField(5, 1), "IN_PNMODE": BitField(4, 1), "IN_INVERSE": BitField(3, 1), "IN_EN": BitField(0, 1)}}
}

# REF_MON layout structure
REF_MON_LAYOUT = {
    "REF_MON_IN_MON_FREQ_CFG": {"offset": 0x000, "fields": {"VLD_INTERVAL": BitField(3, 4), "FREQ_OFFS_LIM": BitField(0, 3)}},
    "REF_MON_IN_MON_FREQ_VLD_INTV": {"offset": 0x001, "fields": {"VLD_INTERVAL_SHORT": BitField(0, 8)}},
    "REF_MON_IN_MON_TRANS_THRESHOLD_0_7": {"offset": 0x002, "fields": {"IN_MON_TRANS_THRESHOLD_0_7": BitField(0, 8)}},
    "REF_MON_IN_MON_TRANS_THRESHOLD_8_15": {"offset": 0x003, "fields": {"IN_MON_TRANS_THRESHOLD_8_15": BitField(0, 8)}},
    "REF_MON_IN_MON_TRANS_PERIOD_0_7": {"offset": 0x004, "fields": {"IN_MON_TRANS_PERIOD_0_7": BitField(0, 8)}},
    "REF_MON_IN_MON_TRANS_PERIOD_8_15": {"offset": 0x005, "fields": {"IN_MON_TRANS_PERIOD_8_15": BitField(0, 8)}},
    "REF_MON_IN_MON_ACT_CFG": {"offset": 0x006, "fields": {"QUAL_TIMER": BitField(5, 2), "DSQUAL_TIMER": BitField(3, 2), "ACT_LIM": BitField(0, 3)}},
    "REF_MON_IN_MON_LOS_TOLERANCE_0_7": {"offset": 0x008, "fields": {"IN_MON_LOS_TOLERANCE_0_7": BitField(0, 8)}},
    "REF_MON_IN_MON_LOS_TOLERANCE_8_15": {"offset": 0x009, "fields": {"IN_MON_LOS_TOLERANCE_8_15": BitField(0, 8)}},
    "REF_MON_IN_MON_LOS_CFG": {"offset": 0x00A, "fields": {"LOS_GAP": BitField(1, 2), "LOS_MARGIN": BitField(0, 1)}},
    "REF_MON_IN_MON_CFG": {"offset": 0x00B, "fields": {"DIV_OR_NON_DIV_CLK_SELECT": BitField(5, 1), "TRANS_DETECTOR_EN": BitField(4, 1), "MASK_ACTIVITY": BitField(3, 1), "MASK_FREQ": BitField(2, 1), "MASK_LOS": BitField(1, 1), "EN": BitField(0, 1)}}
}


# PWM_USER_DATA layout structure
PWM_USER_DATA_LAYOUT = {
    "PWM_USER_DATA_PWM_SRC_ENCODER_ID": {"offset": 0x000, "fields": {"ENCODER_ID": BitField(0, 8)}},
    "PWM_USER_DATA_PWM_DST_DECODER_ID": {"offset": 0x001, "fields": {"DECODER_ID": BitField(0, 8)}},
    "PWM_USER_DATA_PWM_USER_DATA_SIZE": {"offset": 0x002, "fields": {"BYTES": BitField(0, 8)}},
    "PWM_USER_DATA_PWM_USER_DATA_CMD_STS": {"offset": 0x003, "fields": {"COMMAND_STATUS": BitField(0, 8)}}
}

# OUTPUT_TDC_CFG layout structure
OUTPUT_TDC_CFG_LAYOUT = {
    "OUTPUT_TDC_CFG_GBL_0_0_7": {"offset": 0x000, "fields": {"FAST_LOCK_ENABLE_DELAY_0_7": BitField(0, 8)}},
    "OUTPUT_TDC_CFG_GBL_0_8_15": {"offset": 0x001, "fields": {"FAST_LOCK_ENABLE_DELAY_8_15": BitField(0, 8)}},
    "OUTPUT_TDC_CFG_GBL_1_0_7": {"offset": 0x002, "fields": {"FAST_LOCK_DISABLE_DELAY_0_7": BitField(0, 8)}},
    "OUTPUT_TDC_CFG_GBL_1_8_15": {"offset": 0x003, "fields": {"FAST_LOCK_DISABLE_DELAY_8_15": BitField(0, 8)}},
    "OUTPUT_TDC_CFG_GBL_2": {"offset": 0x004, "fields": {"RESERVED": BitField(2, 6), "REF_SEL": BitField(1, 1), "ENABLE": BitField(0, 1)}}
}


# OUTPUT_TDC_0 layout structure
OUTPUT_TDC_LAYOUT = {
    "OUTPUT_TDC_CTRL_0_0_7": {"offset": 0x000, "fields": {"SAMPLES_0_7": BitField(0, 8)}},
    "OUTPUT_TDC_CTRL_0_8_15": {"offset": 0x001, "fields": {"SAMPLES_8_15": BitField(0, 8)}},
    "OUTPUT_TDC_CTRL_1_0_7": {"offset": 0x002, "fields": {"TARGET_PHASE_OFFSET_0_7": BitField(0, 8)}},
    "OUTPUT_TDC_CTRL_1_8_15": {"offset": 0x003, "fields": {"TARGET_PHASE_OFFSET_8_15": BitField(0, 8)}},
    "OUTPUT_TDC_CTRL_2": {"offset": 0x004, "fields": {"ALIGN_TARGET_MASK": BitField(0, 8)}},
    "OUTPUT_TDC_CTRL_3": {"offset": 0x005, "fields": {"TARGET_INDEX": BitField(4, 4), "SOURCE_INDEX": BitField(0, 4)}},
    "OUTPUT_TDC_CTRL_4": {"offset": 0x006, "fields": {"DISABLE_MEASUREMENT_FILTER": BitField(7, 1),
                                                      "ALIGN_THRESHOLD_COUNT": BitField(4, 3),
                                                      "ALIGN_RESET": BitField(3, 1),
                                                      "TYPE": BitField(2, 1),
                                                      "MODE": BitField(1, 1),
                                                      "GO": BitField(0, 1)}}
}


# INPUT_TDC layout structure
INPUT_TDC_LAYOUT = {
    "INPUT_TDC_SDM_FRAC_0_7": {"offset": 0x000, "fields": {"SDM_FRAC_0_7": BitField(0, 8)}},
    "INPUT_TDC_SDM_FRAC_8_15": {"offset": 0x001, "fields": {"SDM_FRAC_8_15": BitField(0, 8)}},
    "INPUT_TDC_SDM_MOD_0_7": {"offset": 0x002, "fields": {"SDM_MOD_0_7": BitField(0, 8)}},
    "INPUT_TDC_SDM_MOD_8_15": {"offset": 0x003, "fields": {"SDM_MOD_8_15": BitField(0, 8)}},
    "INPUT_TDC_FBD_CTRL": {"offset": 0x004, "fields": {"FBD_USER_CONFIG_EN": BitField(7, 1), "FBD_INTEGER": BitField(0, 7)}},
    "INPUT_TDC_CTRL": {"offset": 0x005, "fields": {"SDM_ORDER": BitField(1, 2), "REF_SEL": BitField(0, 1)}}
}


# PWM_SYNC_ENCODER layout structure
PWM_SYNC_ENCODER_LAYOUT = {
    "PWM_SYNC_ENCODER_PAYLOAD_CNFG": {
        "offset": 0x000,
        "fields": {
            "PAYLOAD_CH_EN_7": BitField(7, 1),
            "PAYLOAD_CH_EN_6": BitField(6, 1),
            "PAYLOAD_CH_EN_5": BitField(5, 1),
            "PAYLOAD_CH_EN_4": BitField(4, 1),
            "PAYLOAD_CH_EN_3": BitField(3, 1),
            "PAYLOAD_CH_EN_2": BitField(2, 1),
            "PAYLOAD_CH_EN_1": BitField(1, 1),
            "PAYLOAD_CH_EN_0": BitField(0, 1)
        }
    },
    "PWM_SYNC_ENCODER_PAYLOAD_SQUELCH_CNFG": {
        "offset": 0x001,
        "fields": {
            "PAYLOAD_SQUELCH_7": BitField(7, 1),
            "PAYLOAD_SQUELCH_6": BitField(6, 1),
            "PAYLOAD_SQUELCH_5": BitField(5, 1),
            "PAYLOAD_SQUELCH_4": BitField(4, 1),
            "PAYLOAD_SQUELCH_3": BitField(3, 1),
            "PAYLOAD_SQUELCH_2": BitField(2, 1),
            "PAYLOAD_SQUELCH_1": BitField(1, 1),
            "PAYLOAD_SQUELCH_0": BitField(0, 1)
        }
    },
    "PWM_SYNC_ENCODER_CMD": {
        "offset": 0x002,
        "fields": {
            "PWM_SYNC_PHASE_CORR_DISABLE": BitField(1, 1),
            "PWM_SYNC": BitField(0, 1)
        }
    }
}


# PWM_SYNC_DECODER_0 layout structure
PWM_SYNC_DECODER_LAYOUT = {
    "PWM_SYNC_DECODER_PAYLOAD_CNFG_0": {
        "offset": 0x000,
        "fields": {
            "PAYLOAD_CH_EN_1": BitField(7, 1),
            "SRC_CH_IDX_1": BitField(4, 3),
            "PAYLOAD_CH_EN_0": BitField(3, 1),
            "SRC_CH_IDX_0": BitField(0, 3)
        }
    },
    "PWM_SYNC_DECODER_PAYLOAD_CNFG_1": {
        "offset": 0x001,
        "fields": {
            "PAYLOAD_CH_EN_3": BitField(7, 1),
            "SRC_CH_IDX_3": BitField(4, 3),
            "PAYLOAD_CH_EN_2": BitField(3, 1),
            "SRC_CH_IDX_2": BitField(0, 3)
        }
    },
    "PWM_SYNC_DECODER_PAYLOAD_CNFG_2": {
        "offset": 0x002,
        "fields": {
            "PAYLOAD_CH_EN_5": BitField(7, 1),
            "SRC_CH_IDX_5": BitField(4, 3),
            "PAYLOAD_CH_EN_4": BitField(3, 1),
            "SRC_CH_IDX_4": BitField(0, 3)
        }
    },
    "PWM_SYNC_DECODER_PAYLOAD_CNFG_3": {
        "offset": 0x003,
        "fields": {
            "PAYLOAD_CH_EN_7": BitField(7, 1),
            "SRC_CH_IDX_7": BitField(4, 3),
            "PAYLOAD_CH_EN_6": BitField(3, 1),
            "SRC_CH_IDX_6": BitField(0, 3)
        }
    },
    "PWM_SYNC_DECODER_CMD": {
        "offset": 0x004,
        "fields": {
            "PWM_OUTPUT_SQUELCH": BitField(6, 1),
            "PWM_CO_LOCATED_CR": BitField(5, 1),
            "PWM_SYNC_CR_IDX": BitField(1, 4),
            "PWM_SYNC": BitField(0, 1)
        }
    }
}


class Module:
    def __init__(self, name, layout, read_func, write_func, base_addresses):
        self.name = name
        self.layout = layout
        self.read_func = read_func
        self.write_func = write_func
        self.base_addresses = base_addresses

    def _validate_module_num(self, module_num):
        if module_num not in self.base_addresses:
            raise ValueError(f"Invalid module number: {module_num}")

    def read_field(self, module_num, register_name, field_name):
        self._validate_module_num(module_num)
        base_address = self.base_addresses[module_num]
        reg_info = self.layout[register_name]
        reg_value = self.read_func(base_address + reg_info['offset'])
        return reg_info['fields'][field_name].get_value(reg_value)

    def write_field(self, module_num, register_name, field_name, field_value):
        self._validate_module_num(module_num)
        base_address = self.base_addresses[module_num]
        reg_addr = base_address + reg_info['offset']
        reg_value = self.read_func(reg_addr)
        new_reg_value = reg_info['fields'][field_name].set_value(
            reg_value, field_value)
        self.write_func(reg_addr, new_reg_value)

    def print_configuration(self, module_num):
        self._validate_module_num(module_num)
        base_address = self.base_addresses[module_num]
        for reg_name, reg_info in self.layout.items():
            reg_value = self.read_func(base_address + reg_info['offset'])
            print(
                f"Module {self.name}{module_num}Register {reg_name} (0x{base_address + reg_info['offset']:04X}): 0x{reg_value:08X}")
            for field_name, bit_field in reg_info['fields'].items():
                field_value = bit_field.get_value(reg_value)
                print(f" - {field_name}: {field_value}")

    def print_register(self, module_num, register, detail=False):
        self._validate_module_num(module_num)
        base_address = self.base_addresses[module_num]
        reg_info = self.layout[register]
        reg_value = self.read_func(base_address + reg_info['offset'])
        print(
            f"Module {self.name}{module_num} Register {register} (0x{base_address + reg_info['offset']:04X}): 0x{reg_value:08X}")
        if detail:
            for field_name, bit_field in reg_info['fields'].items():
                field_value = bit_field.get_value(reg_value)
                print(f" - {field_name}: {field_value}")

    def print_all_registers(self, module_num):
        self._validate_module_num(module_num)
        for register in self.layout:
            self.print_register(module_num, register, True)


class PWMEncoder(Module):
    BASE_ADDRESSES = {0: 0xCB00, 1: 0xCB08, 2: 0xCB10,
                      3: 0xCB18, 4: 0xCB20, 5: 0xCB28, 6: 0xCB30, 7: 0xCB38}
    LAYOUT = PWM_ENCODER_LAYOUT

    def __init__(self, read_func, write_func):
        super().__init__("PWM_ENCODER", PWMEncoder.LAYOUT,
                         read_func, write_func, PWMEncoder.BASE_ADDRESSES)


class PWMDecoder(Module):
    BASE_ADDRESSES = {0: 0xCB40, 1: 0xCB48, 2: 0xCB50, 3: 0xCB58, 4: 0xCB60, 5: 0xCB68, 6: 0xCB70, 7: 0xCB80,
                      8: 0xCB88, 9: 0xCB90, 10: 0xCB98, 11: 0xCBA0, 12: 0xCBA8, 13: 0xCBB0, 14: 0xCBB8, 15: 0xCBC0}
    LAYOUT = PWM_DECODER_LAYOUT

    def __init__(self, read_func, write_func):
        super().__init__("PWM_DECODER", PWMDecoder.LAYOUT,
                         read_func, write_func, PWMDecoder.BASE_ADDRESSES)


class TOD(Module):
    BASE_ADDRESSES = {0: 0xCBC8, 1: 0xCBCC, 2: 0xCBD0, 3: 0xCBD2}
    LAYOUT = TOD_LAYOUT

    def __init__(self, read_func, write_func):
        super().__init__("TOD", TOD.LAYOUT, read_func, write_func, TOD.BASE_ADDRESSES)


class TODWrite(Module):
    BASE_ADDRESSES = {0: 0xCC00, 1: 0xCC10, 2: 0xCC20, 3: 0xCC30}
    LAYOUT = TOD_WRITE_LAYOUT

    def __init__(self, read_func, write_func):
        super().__init__("TOD_WRITE", TODWrite.LAYOUT,
                         read_func, write_func, TODWrite.BASE_ADDRESSES)


class TODReadPrimary(Module):
    BASE_ADDRESSES = {0: 0xCC40, 1: 0xCC50, 2: 0xCC60, 3: 0xCC80}
    LAYOUT = TOD_READ_PRIMARY_LAYOUT

    def __init__(self, read_func, write_func):
        super().__init__("TOD_READ_PRIMARY", TODReadPrimary.LAYOUT,
                         read_func, write_func, TODReadPrimary.BASE_ADDRESSES)


class TODReadSecondary(Module):
    BASE_ADDRESSES = {0: 0xCC90, 1: 0xCCA0, 2: 0xCCB0, 3: 0xCCC0}
    LAYOUT = TOD_READ_SECONDARY_LAYOUT

    def __init__(self, read_func, write_func):
        super().__init__("TOD_READ_SECONDARY", TODReadSecondary.LAYOUT,
                         read_func, write_func, TODReadSecondary.BASE_ADDRESSES)


class Input(Module):
    BASE_ADDRESSES = {0: 0xC1B0, 1: 0xC1C0, 2: 0xC1D0, 3: 0xC200, 4: 0xC210, 5: 0xC220, 6: 0xC230, 7: 0xC240,
                      8: 0xC250, 9: 0xC260, 10: 0xC280, 11: 0xC290, 12: 0xC2A0, 13: 0xC2B0, 14: 0xC2C0, 15: 0xC2D0}
    LAYOUT = INPUT_LAYOUT

    def __init__(self, read_func, write_func):
        super().__init__("INPUT", Input.LAYOUT, read_func, write_func, Input.BASE_ADDRESSES)


class REFMON(Module):
    BASE_ADDRESSES = {0: 0xC2E0, 1: 0xC2EC, 2: 0xC300, 3: 0xC30C, 4: 0xC318, 5: 0xC324, 6: 0xC330, 7: 0xC33C,
                      8: 0xC348, 9: 0xC354, 10: 0xC360, 11: 0xC36C, 12: 0xC380, 13: 0xC38C, 14: 0xC398, 15: 0xC3A4}
    LAYOUT = REF_MON_LAYOUT

    def __init__(self, read_func, write_func):
        super().__init__("REF_MON", REFMON.LAYOUT,
                         read_func, write_func, REFMON.BASE_ADDRESSES)

class PWM_USER_DATA(Module):
    BASE_ADDRESSES = {0: 0xCBC8}  # Only one base address for PWM_USER_DATA
    LAYOUT = PWM_USER_DATA_LAYOUT

    def __init__(self, read_func, write_func):
        super().__init__("PWM_USER_DATA", PWM_USER_DATA.LAYOUT,
                         read_func, write_func, PWM_USER_DATA.BASE_ADDRESSES)


class OUTPUT_TDC_CFG(Module):
    BASE_ADDRESSES = {0: 0xCCD0}  # Example of multiple base addresses
    LAYOUT = OUTPUT_TDC_CFG_LAYOUT

    def __init__(self, read_func, write_func):
        super().__init__("OUTPUT_TDC_CFG", OUTPUT_TDC_CFG.LAYOUT,
                         read_func, write_func, OUTPUT_TDC_CFG.BASE_ADDRESSES)


class OUTPUT_TDC(Module):
    BASE_ADDRESSES = {0: 0xCD00, 1: 0xCD08, 2: 0xCD10,
                      3: 0xCD18}  # Example of multiple base addresses
    LAYOUT = OUTPUT_TDC_LAYOUT

    def __init__(self, read_func, write_func):
        super().__init__("OUTPUT_TDC", OUTPUT_TDC.LAYOUT,
                         read_func, write_func, OUTPUT_TDC.BASE_ADDRESSES)


class INPUT_TDC(Module):
    BASE_ADDRESSES = {0: 0xCD20}  # Only one base address for INPUT_TDC
    LAYOUT = INPUT_TDC_LAYOUT

    def __init__(self, read_func, write_func):
        super().__init__("INPUT_TDC", INPUT_TDC.LAYOUT,
                         read_func, write_func, INPUT_TDC.BASE_ADDRESSES)


class PWM_SYNC_ENCODER(Module):
    BASE_ADDRESSES = {0: 0xCD80, 1: 0xCD84, 2: 0xCD88, 3: 0xCD8C, 4: 0xCD90,
                      5: 0xCD94, 6: 0xCD98, 7: 0xCD9C}  # Example of multiple base addresses
    LAYOUT = PWM_SYNC_ENCODER_LAYOUT

    def __init__(self, read_func, write_func):
        super().__init__("PWM_SYNC_ENCODER", PWM_SYNC_ENCODER.LAYOUT,
                         read_func, write_func, PWM_SYNC_ENCODER.BASE_ADDRESSES)


class PWM_SYNC_DECODER(Module):
    BASE_ADDRESSES = {
        0: 0xCE00, 1: 0xCE06, 2: 0xCE0C, 3: 0xCE12,
        4: 0xCE18, 5: 0xCE1E, 6: 0xCE24, 7: 0xCE2A,
        8: 0xCE30, 9: 0xCE36, 10: 0xCE3C, 11: 0xCE42,
        12: 0xCE48, 13: 0xCE4E, 14: 0xCE54, 15: 0xCE5A
    }
    LAYOUT = PWM_SYNC_DECODER_LAYOUT

    def __init__(self, read_func, write_func):
        super().__init__("PWM_SYNC_DECODER", PWM_SYNC_DECODER.LAYOUT,
                         read_func, write_func, PWM_SYNC_DECODER.BASE_ADDRESSES)


# holder of registers
class DPLL():
    def __init__(self, i2c_dev, read_func, write_func):
        # make modules inside the dpll
        self.modules = {}

        modules_to_use = [PWMEncoder, PWMDecoder, TOD, TODWrite, TODReadPrimary,
                          TODReadSecondary, Input, REFMON, PWM_USER_DATA,
                          OUTPUT_TDC_CFG, OUTPUT_TDC, INPUT_TDC, PWM_SYNC_ENCODER,
                          PWM_SYNC_DECODER]

        for mod in modules_to_use:
            self.modules[mod.__name__] = mod(read_func, write_func)
        self.gpio = cm_gpios(i2c_dev)


def parse_dpll_config_file(file_path):
    """
    Parses the DPLL configuration file to extract register addresses and values.
    Expects lines in the format: "C0.0X 00000000 00 C0.0X"
    """
    config_data = []
    pattern = re.compile(
        r'C0\.[0-9A-F]{2}\s+00000000\s+[0-9A-F]{2}\s+C0\.[0-9A-F]{2}')

    with open(file_path, 'r') as file:
        for line in file:
            match = pattern.search(line)
            if match:
                parts = line.split()
                # Extract the register address and value
                register = int(parts[0].replace('C0.', ''), 16)
                value = int(parts[2], 16)
                config_data.append((register, value))

    return config_data



if __name__ == "__main__": 
    # Define the file path
    file_path = '/mnt/data/8A34012_20230901_123654_CLK3_PWMpos_v8d.tcs'

    # Parse the configuration file
    parsed_config = parse_dpll_config_file(file_path)

    # Display the first few parsed configurations for verification
    parsed_config[:10]  # Show first 10 configurations
