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


# OUTPUT layout structure
OUTPUT_LAYOUT = {
    "OUT_PHASE_ADJ_7_0": {"offset": 0x00c, "fields": {"Value": BitField(0, 8)}},
    "OUT_PHASE_ADJ_15_8": {"offset": 0x00d, "fields": {"Value": BitField(0, 8)}},
    "OUT_PHASE_ADJ_23_16": {"offset": 0x00e, "fields": {"Value": BitField(0, 8)}},
    "OUT_PHASE_ADJ_31_24": {"offset": 0x00f, "fields": {"Value": BitField(0, 8)}},
    "OUT_CTRL_1":{"offset": 0x9, "fields": {"Value": BitField(0,8)}},

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


# Common field structures
MON_STATUS_FIELDS = {
    "TRANS_DETECT_STICKY": BitField(7, 1),
    "FREQ_OFFS_LIM_STICKY": BitField(6, 1),
    "NO_ACTIVITY_STICKY": BitField(5, 1),
    "LOS_STICKY": BitField(4, 1),
    "TRANS_DETECT_LIVE": BitField(3, 1),
    "FREQ_OFFS_LIM_LIVE": BitField(2, 1),
    "NO_ACTIVITY_LIVE": BitField(1, 1),
    "LOS_LIVE": BitField(0, 1)
}

DPLL_STATUS_FIELDS = {
    "RESERVED": BitField(6, 2),
    "HOLDOVER_STATE_CHANGE_STICKY": BitField(5, 1),
    "LOCK_STATE_CHANGE_STICKY": BitField(4, 1),
    "DPLL_STATE": BitField(0, 4)
}

SYS_APLL_STATUS_FIELDS = {
    "RESERVED_1": BitField(5, 3),
    "LOSS_LOCK_STICKY": BitField(4, 1),
    "RESERVED_2": BitField(1, 3),
    "LOSS_LOCK_LIVE": BitField(0, 1)
}

DPLL_REF_STATUS_FIELDS = {
    "RESERVED": BitField(5, 3),
    "DPLL_INPUT": BitField(0, 5)
}


# STATUS module layout structure
STATUS_LAYOUT = {
    # I2CM and SER statuses
    "I2CM_STATUS": {"offset": 0x000, "fields": {"RESERVED": BitField(4, 4), "I2CM_SPEED": BitField(2, 2), "I2CM_PORT_SEL": BitField(0, 2)}},
    "SER0_STATUS": {"offset": 0x002, "fields": {"RESERVED": BitField(3, 5), "ADDRESS_SIZE": BitField(2, 1), "MODE": BitField(0, 2)}},
    "SER0_SPI_STATUS": {"offset": 0x003, "fields": {"RESERVED": BitField(5, 3), "SPI_SDO_DELAY": BitField(4, 1), "SPI_CLOCK_SELECTION": BitField(3, 1), "SPI_DUPLEX_MODE": BitField(2, 1), "RESERVED_0": BitField(0, 2)}},
    "SER0_I2C_STATUS": {"offset": 0x004, "fields": {"RESERVED": BitField(7, 1), "DEVICE_ADDRESS": BitField(0, 7)}},
    "SER1_STATUS": {"offset": 0x005, "fields": {"RESERVED": BitField(3, 5), "ADDRESS_SIZE": BitField(2, 1), "MODE": BitField(0, 2)}},
    "SER1_SPI_STATUS": {"offset": 0x006, "fields": {"RESERVED": BitField(5, 3), "SPI_SDO_DELAY": BitField(4, 1), "SPI_CLOCK_SELECTION": BitField(3, 1), "SPI_DUPLEX_MODE": BitField(2, 1), "RESERVED_0": BitField(0, 2)}},
    "SER1_I2C_STATUS": {"offset": 0x007, "fields": {"RESERVED": BitField(7, 1), "DEVICE_ADDRESS": BitField(0, 7)}},

    # IN_MON_STATUS for inputs 0 to 15
    **{f"IN{num}_MON_STATUS": {"offset": 0x008 + num, "fields": MON_STATUS_FIELDS} for num in range(16)},


    # SYS_DPLL
    "SYS_DPLL": {"offset": 0x020, "fields": {"RESERVED": BitField(6, 2), "DPLL_SYS_HOLDOVER_STATE_CHANGE_STICKY": BitField(5, 1), "DPLL_SYS_LOCK_STATE_CHANGE_STICKY": BitField(4, 1), "DPLL_SYS_STATE": BitField(0, 4)}},

    # DPLL_REF_STATUS for each DPLL and DPLL_SYS
    **{f"DPLL{num}_REF_STATUS": {"offset": 0x022 + num, "fields": {"RESERVED": BitField(5, 3), f"DPLL{num}_INPUT": BitField(0, 5)}}
       for num in range(8)},
    "DPLL_SYS_REF_STATUS": {"offset": 0x02A, "fields": {"RESERVED": BitField(5, 3), "DPLL_SYS_INPUT": BitField(0, 5)}},

    # DPLL_FILTER_STATUS registers for DPLL0 to DPLL7 and DPLL_SYS
    **{f"DPLL{num}_FILTER_STATUS_{7 + i*8}_{i*8}": {"offset": 0x044 + num * 8 + i, "fields": {f"FILTER_STATUS_{7 + i*8}_{i*8}": BitField(0, 8)}}
       for num in range(8) for i in range(6)},
    **{f"DPLL_SYS_FILTER_STATUS_{7 + i*8}_{i*8}": {"offset": 0x084 + i, "fields": {f"FILTER_STATUS_{7 + i*8}_{i*8}": BitField(0, 8)}}
       for i in range(6)},

    # GPIO STATUS registers
    "USER_GPIO0_TO_7_STATUS": {"offset": 0x08A, "fields": {f"GPIO{i}_LEVEL": BitField(i, 1) for i in range(8)}},
    "USER_GPIO8_TO_15_STATUS": {"offset": 0x08B, "fields": {f"GPIO{i + 8}_LEVEL": BitField(i, 1) for i in range(8)}},

    # IN_MON_FREQ_STATUS registers
    **{f"IN{num}_MON_FREQ_STATUS_0": {"offset": 0x08C + num * 2, "fields": {"FFO_7_0": BitField(0, 8)}}
       for num in range(16)},
    **{f"IN{num}_MON_FREQ_STATUS_1": {"offset": 0x08D + num * 2, "fields": {"FFO_UNIT": BitField(6, 2), f"FFO_13:8": BitField(0, 6)}}
       for num in range(16)},


}
# Continuing STATUS module layout structure
STATUS_LAYOUT.update({

    # DPLL_PHASE_PULL_IN_STATUS registers
    **{f"DPLL{num}_PHASE_PULL_IN_STATUS": {"offset": 0x11C + num, "fields": {"REMAINING_TIME": BitField(0, 8)}}
       for num in range(8)},

    # OUTPUT_TDC_CFG_STATUS and OUTPUT_TDCn_STATUS registers
    "OUTPUT_TDC_CFG_STATUS": {"offset": 0x0AC, "fields": {"RESERVED": BitField(2, 6), "STATE": BitField(0, 2)}},
    **{f"OUTPUT_TDC{num}_STATUS": {"offset": 0x0AD + num, "fields": {"VALID": BitField(7, 1), "RESERVED": BitField(4, 3), "STATUS": BitField(0, 4)}}
       for num in range(4)},

    # DPLL status registers from 0x18 to 0x1f
    **{f"DPLL{num}_STATUS": {"offset": 0x018 + num, "fields": DPLL_STATUS_FIELDS} for num in range(8)},

    # SYS_APLL_STATUS
    "SYS_APLL_STATUS": {"offset": 0x021, "fields": SYS_APLL_STATUS_FIELDS},

    # OUTPUT_TDC_MEASUREMENT blocks
    **{f"OUTPUT_TDC{num}_MEASUREMENT_{7 + i*8}_{i*8}": {"offset": 0x0B4 + num * 16 + i, "fields": {f"PHASE_{7 + i*8}_{i*8}": BitField(0, 8)}}
       for num in range(4) for i in range(6)},

})


# Additional registers and fields can be added as needed
# Adjusted DPLL_PHASE_STATUS and DPLL_PHASE_PULL_IN_STATUS registers

# DPLL_PHASE_STATUS registers
for num in range(8):
    STATUS_LAYOUT.update({
        f"DPLL{num}_PHASE_STATUS_7_0": {"offset": 0x0DC + num * 8, "fields": {"PHASE_7_0": BitField(0, 8)}},
        f"DPLL{num}_PHASE_STATUS_15_8": {"offset": 0x0DD + num * 8, "fields": {"PHASE_15_8": BitField(0, 8)}},
        f"DPLL{num}_PHASE_STATUS_23_16": {"offset": 0x0DE + num * 8, "fields": {"PHASE_23_16": BitField(0, 8)}},
        f"DPLL{num}_PHASE_STATUS_31_24": {"offset": 0x0DF + num * 8, "fields": {"PHASE_31_24": BitField(0, 8)}},
        f"DPLL{num}_PHASE_STATUS_35_32": {"offset": 0x0E0 + num * 8, "fields": {"PHASE_35_32": BitField(0, 4)}},
    })

# DPLL_PHASE_PULL_IN_STATUS registers
for num in range(8):
    STATUS_LAYOUT.update({
        f"DPLL{num}_PHASE_PULL_IN_STATUS": {"offset": 0x11C + num, "fields": {"REMAINING_TIME": BitField(0, 8)}}
    })

EEPROM_LAYOUT = {
    "EEPROM_I2C_ADDR": {"offset": 0x000, "fields": {"RESERVED": BitField(7, 1), "I2C_ADDR": BitField(0, 7)}},
    "EEPROM_SIZE": {"offset": 0x001, "fields": {"BYTES": BitField(0, 8)}},
    "EEPROM_OFFSET_LOW": {"offset": 0x002, "fields": {"EEPROM_OFFSET": BitField(0, 8)}},
    "EEPROM_OFFSET_HIGH": {"offset": 0x003, "fields": {"EEPROM_OFFSET": BitField(0, 8)}},
    "EEPROM_CMD_LOW": {"offset": 0x004, "fields": {"EEPROM_CMD": BitField(0, 8)}},
    "EEPROM_CMD_HIGH": {"offset": 0x005, "fields": {"EEPROM_CMD": BitField(0, 8)}}
}

BYTE_BUFFER_LAYOUT = {
    **{f"BYTE_OTP_EEPROM_PWM_BUFF_{i}": {"offset": 0x000 + i, "fields": {"DATA": BitField(0, 8)}}
       for i in range(128)}
}


PWM_RX_INFO_LAYOUT = {
    "PWM_TOD_SUBNS": {"offset": 0x000, "fields": {"VALUE": BitField(0, 8)}},
    "PWM_TOD_NS_7_0": {"offset": 0x001, "fields": {"VALUE": BitField(0, 8)}},
    "PWM_TOD_NS_15_8": {"offset": 0x002, "fields": {"VALUE": BitField(0, 8)}},
    "PWM_TOD_NS_23_16": {"offset": 0x003, "fields": {"VALUE": BitField(0, 8)}},
    "PWM_TOD_NS_31_24": {"offset": 0x004, "fields": {"VALUE": BitField(0, 8)}},
    "PWM_TOD_SEC_7_0": {"offset": 0x005, "fields": {"VALUE": BitField(0, 8)}},
    "PWM_TOD_SEC_15_8": {"offset": 0x006, "fields": {"VALUE": BitField(0, 8)}},
    "PWM_TOD_SEC_23_16": {"offset": 0x007, "fields": {"VALUE": BitField(0, 8)}},
    "PWM_TOD_SEC_31_24": {"offset": 0x008, "fields": {"VALUE": BitField(0, 8)}},
    "PWM_TOD_SEC_39_32": {"offset": 0x009, "fields": {"VALUE": BitField(0, 8),
                                                      "PWM_RandID": BitField(0, 8)}},
    "PWM_TOD_SEC_47_40": {"offset": 0x00a, "fields": {"VALUE": BitField(0, 8)},
                          "DataFlag": BitField(7, 1),
                          "HandshakeData": BitField(5, 2),
                          "PWM_Transaction_ID": BitField(0, 5),
                          },
}


DPLL_CTRL_LAYOUT = {
    "DPLL_DECIMATOR_BW_MULT": {"offset": 0x3, "fields": {"VALUE": BitField(0,8)}},
    "DPLL_BW_0": {"offset": 0x4, "fields": {"BW_7_0": BitField(0,8)}},
    "DPLL_BW_1": {"offset": 0x5, "fields": {"BW_13_8": BitField(0, 6) , "BW_UNIT": BitField(6,2)}},
    "DPLL_PSL_7_0": {"offset": 0x6, "fields": {"VALUE": BitField(0,8)}},
    "DPLL_PSL_15_8": {"offset": 0x7, "fields": {"VALUE": BitField(0,8)}},
    "DPLL_PHASE_OFFSET_CFG_7_0": {"offset": 0x14, "fields": {"VALUE": BitField(0,8)}},
    "DPLL_PHASE_OFFSET_CFG_15_8": {"offset": 0x15, "fields": {"VALUE": BitField(0,8)}},
    "DPLL_PHASE_OFFSET_CFG_23_16": {"offset": 0x16, "fields": {"VALUE": BitField(0,8)}},
    "DPLL_PHASE_OFFSET_CFG_31_24": {"offset": 0x17, "fields": {"VALUE": BitField(0,8)}},
    "DPLL_PHASE_OFFSET_CFG_35_32": {"offset": 0x18, "fields": {"VALUE": BitField(0,4)}},
    "DPLL_FINE_PHASE_ADV_CFG_7_0": {"offset": 0x1a, "fields": {"VALUE": BitField(0,8)}},
    "DPLL_FINE_PHASE_ADV_CFG_12_8": {"offset": 0x1b, "fields": {"VALUE": BitField(0,5)}},

    "FOD_FREQ_M_7_0": {"offset": 0x1c, "fields": {"VALUE": BitField(0, 8)}},
    "FOD_FREQ_M_15_8": {"offset": 0x1d, "fields": {"VALUE": BitField(0, 8)}},
    "FOD_FREQ_M_23_16": {"offset": 0x1e, "fields": {"VALUE": BitField(0, 8)}},
    "FOD_FREQ_M_31_24": {"offset": 0x1f, "fields": {"VALUE": BitField(0, 8)}},
    "FOD_FREQ_M_39_32": {"offset": 0x20, "fields": {"VALUE": BitField(0, 8)}},
    "FOD_FREQ_M_47_40": {"offset": 0x21, "fields": {"VALUE": BitField(0, 8)}},
    "FOD_FREQ_N_7_0": {"offset": 0x22, "fields": {"VALUE": BitField(0, 8)}},
    "FOD_FREQ_N_15_8": {"offset": 0x23, "fields": {"VALUE": BitField(0, 8)}},

    "DPLL_FRAME_PULSE_SYNC": {"offset":0x3b, "fields": {"VALUE": BitField(0,1)}},
}


DPLL_FREQ_WRITE_LAYOUT = {
    "DPLL_WR_FREQ_7_0": {"offset": 0x0, "fields": {"VALUE": BitField(0, 8)}},
    "DPLL_WR_FREQ_15_8": {"offset": 0x1, "fields": {"VALUE": BitField(0, 8)}},
    "DPLL_WR_FREQ_23_16": {"offset": 0x2, "fields": {"VALUE": BitField(0, 8)}},
    "DPLL_WR_FREQ_31_24": {"offset": 0x3, "fields": {"VALUE": BitField(0, 8)}},
    "DPLL_WR_FREQ_39_32": {"offset": 0x4, "fields": {"VALUE": BitField(0, 8)}},
    "DPLL_WR_FREQ_41_40": {"offset": 0x5, "fields": {"VALUE": BitField(0, 2),
                                                     "Reserved": BitField(2, 6)
                                                     }
                           },
}


DPLL_GENERAL_STATUS_LAYOUT = {
    "EEPROM_STATUS_7_0": {"offset": 0x8, "fields": {"VALUE": BitField(0, 8)}},
    "EEPROM_STATUS_8_15": {"offset": 0x9, "fields": {"VALUE": BitField(0, 8)}},
    "MAJOR RELEASE": {"offset": 0x10, "fields": {"VALUE": BitField(0, 8)}},
    "MINOR RELEASE": {"offset": 0x11, "fields": {"VALUE": BitField(0, 8)}},
    "HOTFIX RELEASE": {"offset": 0x12, "fields": {"VALUE": BitField(0, 8)}},
    "JTAG DEVICE ID": {"offset": 0x1c, "fields": {"VALUE": BitField(0, 8)}},
    "PRODUCT ID": {"offset": 0x1e, "fields": {"VALUE": BitField(0, 8)}},
}


# Common bitfield structure for DPLL_REF_PRIORITY registers
DPLL_REF_PRIORITY_FIELDS = {
    "PRIORITY_GROUP_NUMBER": BitField(6, 2),
    "PRIORITY_REF": BitField(1, 5),
    "PRIORITY_EN": BitField(0, 1)
}

DPLL_LAYOUT = {
    "DPLL_DCO_INC_DEC_SIZE_7_0": {"offset": 0x000, "fields": {"DCO_INC_DEC_SIZE_7_0": BitField(0, 8)}},
    "DPLL_DCO_INC_DEC_SIZE_15_8": {"offset": 0x001, "fields": {"DCO_INC_DEC_SIZE_15_8": BitField(0, 8)}},
    "DPLL_CTRL_0": {"offset": 0x002, "fields": {"FORCE_LOCK_INPUT": BitField(3, 5), "GLOBAL_SYNC_EN": BitField(2, 1), "REVERTIVE_EN": BitField(1, 1), "HITLESS_EN": BitField(0, 1)}},
    "DPLL_CTRL_1": {"offset": 0x003, "fields": {"HITLESS_TYPE": BitField(5, 1), "FB_SELECT_REF": BitField(1, 4), "FB_SELECT_REF_EN": BitField(0, 1)}},
    "DPLL_CTRL_2": {"offset": 0x004, "fields": {"FRAME_SYNC_PULSE_RESYNC_EN": BitField(7, 1), "FRAME_SYNC_MODE": BitField(5, 2), "EXT_FB_REF_SELECT": BitField(1, 4), "EXT_FB_EN": BitField(0, 1)}},
    "DPLL_UPDATE_RATE_CFG": {"offset": 0x005, "fields": {"UPDATE_RATE_CFG": BitField(0, 2)}},
    "DPLL_FILTER_STATUS_UPDATE_CFG": {"offset": 0x006, "fields": {"FILTER_STATUS_UPDATE_EN": BitField(2, 1), "FILTER_STATUS_SELECT_CNFG": BitField(0, 2)}},
    "DPLL_HO_ADVCD_HISTORY": {"offset": 0x007, "fields": {"HISTORY": BitField(0, 6)}},
    "DPLL_HO_ADVCD_BW_7_0": {"offset": 0x008, "fields": {"DPLL_HO_ADVCD_BW_7_0": BitField(0, 8)}},
    "DPLL_HO_ADVCD_BW_15_8": {"offset": 0x009, "fields": {"BW_UNIT": BitField(6, 2), "DPLL_HO_ADVCD_BW_15_8": BitField(0, 6)}},
    "DPLL_HO_CFG": {"offset": 0x00A, "fields": {"HOLDOVER_MODE": BitField(0, 3)}},
    "DPLL_LOCK_0": {"offset": 0x00B, "fields": {"PHASE_UNIT": BitField(6, 2), "PHASE_LOCK_MAX_ERROR": BitField(0, 6)}},
    "DPLL_LOCK_1": {"offset": 0x00C, "fields": {"PHASE_MON_DUR": BitField(0, 8)}},
    "DPLL_LOCK_2": {"offset": 0x00D, "fields": {"FFO_UNIT": BitField(6, 2), "FFO_LOCK_MAX_ERROR": BitField(0, 6)}},
    "DPLL_LOCK_3": {"offset": 0x00E, "fields": {"FFO_MON_DUR": BitField(0, 8)}},

    # Generating DPLL_REF_PRIORITY registers
    **{f"DPLL_REF_PRIORITY_{i}": {"offset": 0x00F + i, "fields": {
        "PRIORITY_GROUP_NUMBER": BitField(6, 2),
        "PRIORITY_REF": BitField(1, 5),
        "PRIORITY_EN": BitField(0, 1)
    }} for i in range(19)},

    # Next five registers
    "DPLL_TRANS_CTRL": {"offset": 0x022, "fields": {
        "RESERVED": BitField(2, 6),
        "TRANS_SUPPRESS_EN": BitField(1, 1),
        "TRANS_DETECT_EN": BitField(0, 1)
    }},
    "DPLL_FASTLOCK_CFG_0": {"offset": 0x023, "fields": {
        "LOCK_REC_PULL_IN_EN": BitField(7, 1),
        "LOCK_REC_FAST_ACQ_EN": BitField(6, 1),
        "LOCK_REC_PHASE_SNAP_EN": BitField(5, 1),
        "LOCK_REC_FREQ_SNAP_EN": BitField(4, 1),
        "LOCK_ACQ_PULL_IN_EN": BitField(3, 1),
        "LOCK_ACQ_FAST_ACQ_EN": BitField(2, 1),
        "LOCK_ACQ_PHASE_SNAP_EN": BitField(1, 1),
        "LOCK_ACQ_FREQ_SNAP_EN": BitField(0, 1)
    }},
    "DPLL_FASTLOCK_CFG_1": {"offset": 0x024, "fields": {
        "PRE_FAST_ACQ_TIMER": BitField(4, 4),
        "DAMP_FTR": BitField(0, 4)
    }},
    "DPLL_MAX_FREQ_OFFSET": {"offset": 0x025, "fields": {
        "MAX_FFO": BitField(0, 8)
    }},
    "DPLL_FASTLOCK_PSL": {"offset": 0x026, "fields": {
        "DPLL_FASTLOCK_PSL_7_0": BitField(0, 8)
    }},
    "DPLL_FASTLOCK_PSL_15_8": {"offset": 0x027, "fields": {
        "DPLL_FASTLOCK_PSL_15_8": BitField(0, 8)
    }},
    "DPLL_FASTLOCK_FSL": {"offset": 0x028, "fields": {
        "DPLL_FASTLOCK_FSL_7_0": BitField(0, 8)
    }},
    "DPLL_FASTLOCK_FSL_15_8": {"offset": 0x029, "fields": {
        "DPLL_FASTLOCK_FSL_15_8": BitField(0, 8)
    }},
    "DPLL_FASTLOCK_BW": {"offset": 0x02A, "fields": {
        "DPLL_FASTLOCK_BW_7_0": BitField(0, 8)
    }},
    "DPLL_FASTLOCK_BW_15_8": {"offset": 0x02B, "fields": {
        "BW_UNIT": BitField(6, 2),
        "DPLL_FASTLOCK_BW_15_8": BitField(0, 6)
    }},
    "DPLL_WRITE_FREQ_TIMER": {"offset": 0x02C, "fields": {
        "WRITE_FREQ_TIMEOUT_CNFG_7_0": BitField(0, 8)
    }},
    "DPLL_WRITE_FREQ_TIMER_15_8": {"offset": 0x02D, "fields": {
        "WRITE_FREQ_TIMEOUT_CNFG_15_8": BitField(0, 8)
    }},
    "DPLL_WRITE_PHASE_TIMER": {"offset": 0x02E, "fields": {
        "WRITE_PHASE_TIMEOUT_CNFG_7_0": BitField(0, 8)
    }},
    "DPLL_WRITE_PHASE_TIMER_15_8": {"offset": 0x02F, "fields": {
        "WRITE_PHASE_TIMEOUT_CNFG_15_8": BitField(0, 8)
    }},
    "DPLL_PRED_CFG": {"offset": 0x030, "fields": {
        "RESERVED": BitField(2, 6),
        "WP_PRED": BitField(1, 1),
        "PRED_EN": BitField(0, 1)
    }},
    "DPLL_TOD_SYNC_CFG": {"offset": 0x031, "fields": {
        "RESERVED": BitField(3, 5),
        "TOD_SYNC_SOURCE": BitField(1, 2),
        "TOD_SYNC_EN": BitField(0, 1)
    }},
    "DPLL_COMBO_SLAVE_CFG_0": {"offset": 0x032, "fields": {
        "RESERVED": BitField(5, 3),
        "PRI_COMBO_SRC_EN": BitField(5, 1),
        "PRI_COMBO_SRC_FILTERED_CNFG": BitField(4, 1),
        "PRI_COMBO_SRC_ID": BitField(0, 4)
    }},
    "DPLL_COMBO_SLAVE_CFG_1": {"offset": 0x033, "fields": {
        "RESERVED": BitField(5, 3),
        "SEC_COMBO_SRC_EN": BitField(5, 1),
        "SEC_COMBO_SRC_FILTERED_CNFG": BitField(4, 1),
        "SEC_COMBO_SRC_ID": BitField(0, 4)
    }},
    "DPLL_SLAVE_REF_CFG": {"offset": 0x034, "fields": {
        "RESERVED": BitField(4, 4),
        "SLAVE_REFERENCE": BitField(0, 4)
    }},
    "DPLL_REF_MODE": {"offset": 0x035, "fields": {
        "RESERVED": BitField(3, 5),
        "MODE": BitField(0, 3)
    }},
    "DPLL_PHASE_MEASUREMENT_CFG": {"offset": 0x036, "fields": {
        "PFD_FB_CLK_SEL": BitField(4, 4),
        "PFD_REF_CLK_SEL": BitField(0, 4)
    }},
    "DPLL_MODE": {"offset": 0x037, "fields": {
        "WRITE_TIMER_MODE": BitField(6, 1),
        "PLL_MODE": BitField(3, 3),
        "STATE_MODE": BitField(0, 3)
    }}
}


def int_to_signed_nbit(number, n_bits):
    """
    Interpret an integer as an n-bit signed integer and return its decimal equivalent.

    :param number: The integer to be interpreted
    :param n_bits: The bit size of the signed integer
    :return: Decimal equivalent of the n-bit signed integer
    """
    # Mask to extract n_bits
    mask = (1 << n_bits) - 1
    number = number & mask

    # Check if negative (if MSB is 1)
    is_negative = (number >> (n_bits - 1)) & 1

    # Function to calculate two's complement for negative numbers
    def twos_complement(binary_str):
        # Invert the bits
        inverted = ''.join('1' if b == '0' else '0' for b in binary_str)
        # Add 1
        decimal = int(inverted, 2) + 1
        return -1 * decimal

    binary_n_bit = format(number, f'0{n_bits}b')

    # Convert to decimal
    if is_negative:
        return twos_complement(binary_n_bit)
    else:
        return int(binary_n_bit, 2)


def hex_to_signed_nbit(hex_value, n_bits):
    """
    Convert a hexadecimal string value assumed to be an n-bit signed integer
    to its decimal equivalent.

    :param hex_value: String representing a hexadecimal number
    :param n_bits: The bit size of the signed integer
    :return: Decimal equivalent of the n-bit signed integer
    """
    # Convert to binary (full length)
    full_binary = bin(int(hex_value, 16))[2:].zfill(n_bits)

    # Extracting the relevant bits (considering n_bits)
    binary_n_bit = full_binary[-n_bits:]

    # Check if negative (if MSB is 1)
    is_negative = binary_n_bit[0] == '1'

    # Function to calculate two's complement for negative numbers
    def twos_complement(binary_str):
        # Invert the bits
        inverted = ''.join('1' if b == '0' else '0' for b in binary_str)
        # Add 1
        decimal = int(inverted, 2) + 1
        return -decimal

    # Convert to decimal
    if is_negative:
        return twos_complement(binary_n_bit)
    else:
        return int(binary_n_bit, 2)


def to_twos_complement_bytes(value, n_bits):
    # Adjust value for n-bit representation
    if value < 0:
        value = (1 << n_bits) + value
    else:
        value = value % (1 << n_bits)

    # Calculate the number of bytes needed for n bits
    n_bytes = (n_bits + 7) // 8

    # Convert to bytes
    value_in_bytes = value.to_bytes(n_bytes, byteorder='little', signed=False)

    # Convert each byte to an integer and return the list
    return [byte for byte in value_in_bytes]


def calculate_fcw(ffo_ppm=0):
    ffo_fraction = ffo_ppm / 10**6
    fcw = (1 - (1 / (1 + ffo_fraction))) * 2**53
    fcw = int(fcw)
    return fcw


def time_to_nanoseconds(time):
    sub_nanoseconds = time[0] / 256  # Convert sub-nanoseconds to nanoseconds
    nanoseconds = int.from_bytes(time[1:5], byteorder='little', signed=False)
    # Convert seconds to nanoseconds
    seconds = int.from_bytes(time[5:], byteorder='little', signed=False) * 1e9
    return sub_nanoseconds + nanoseconds + seconds


def nanoseconds_to_time(nanoseconds):
    seconds = int(nanoseconds // 1e9)
    nanoseconds_remainder = int(nanoseconds % 1e9)
    sub_nanoseconds = int((nanoseconds_remainder % 1) * 256)

    seconds_bytes = seconds.to_bytes(4, byteorder='little', signed=False)
    nanoseconds_bytes = nanoseconds_remainder.to_bytes(
        4, byteorder='little', signed=False)
    return [sub_nanoseconds] + list(nanoseconds_bytes) + list(seconds_bytes)


def time_divide_by_value(time1, value):
    nano_time = time_to_nanoseconds(time1)
    nano_time = nano_time / value
    return nanoseconds_to_time(nanoseconds)

# takes two TOD lists


def time_difference_signed_nanoseconds(time1, time2, include_decoder=False, encoder_freq=25e6):
    # Compute the difference
    diff_nanoseconds = time_to_nanoseconds(time1) - time_to_nanoseconds(time2)
    if ( include_decoder ):
        # PWM delays , 118 cycles at carrier
        diff_nanoseconds -= ( 118 * ( 1 / encoder_freq )) * 1e9
    return diff_nanoseconds

def time_difference_with_flag(time1, time2, include_decoder=False, encoder_freq=25e6):
    """
    Compute the absolute difference between two time values in the specified 11-byte format.
    Also, return a flag indicating which time value was larger.
    Each time value is a list of 11 integers.
    """

    # Compute the difference
    diff_nanoseconds = time_to_nanoseconds(time1) - time_to_nanoseconds(time2)

    if ( include_decoder ):
        # PWM delays , 118 cycles at carrier
        diff_nanoseconds -= ( 118 * ( 1 / encoder_freq )) * 1e9

    # Determine the flag
    if diff_nanoseconds > 0:
        flag = "time1 is larger"
        flag = 1
    elif diff_nanoseconds < 0:
        flag = "time2 is larger"
        flag = -1
    else:
        flag = "times are equal"
        flag = 0

    # Convert the absolute difference back to the 11-byte format
    abs_diff = nanoseconds_to_time(abs(diff_nanoseconds))

    return abs_diff, flag


class Module:
    def __init__(self, name, layout, base_addresses):
        self.name = name
        self.layout = layout
        # self.read_func = read_func
        # self.read_mul_func = read_mul_func
        # self.write_func = write_func
        # self.write_mul_func = write_mul_func
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
        # print(f"Write field {module_num} reg {register_name} {field_name} {field_value}")
        base_address = self.base_addresses[module_num]
        reg_info = self.layout[register_name]
        reg_addr = base_address + reg_info['offset']

        # print(f"Read reg_addr {reg_addr:02x}")
        reg_value = self.read_func(reg_addr)
        # print(f"Read reg_addr {reg_addr:02x} = {reg_value:02x}")
        new_reg_value = reg_info['fields'][field_name].set_value(
            reg_value, field_value)
        self.write_func(reg_addr, new_reg_value)

    def write_reg(self, module_num, register_name, value):
        self._validate_module_num(module_num)
        base_address = self.base_addresses[module_num]
        reg_info = self.layout[register_name]
        reg_addr = base_address + reg_info['offset']
        self.write_func(reg_addr, value)

    def write_reg_mul(self, module_num, register_name, data):
        self._validate_module_num(module_num)
        base_address = self.base_addresses[module_num]
        reg_info = self.layout[register_name]
        reg_addr = base_address + reg_info['offset']
        self.write_mul_func(reg_addr, data)

    def read_reg(self, module_num, register_name):
        self._validate_module_num(module_num)
        base_address = self.base_addresses[module_num]
        reg_info = self.layout[register_name]
        reg_value = self.read_func(base_address + reg_info['offset'])
        return reg_value

    def read_reg_mul(self, module_num, start_reg, length):
        self._validate_module_num(module_num)
        base_address = self.base_addresses[module_num]
        reg_info = self.layout[start_reg]
        reg_value = (base_address + reg_info['offset'])
        # print(f"Read reg mul mod={module_num} base={base_address:02x} start={reg_value} len={length}")
        return self.read_mul_func(reg_value, length)

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
                print(f" - {field_name}: 0x{field_value:x}")

    def print_all_registers(self, module_num):
        self._validate_module_num(module_num)
        for register in self.layout:
            self.print_register(module_num, register, True)

    def print_all_registers_all_modules(self):
        for i in self.BASE_ADDRESSES.keys():
            for register in self.layout:
                self.print_register(i, register, True)


class Status(Module):
    BASE_ADDRESSES = {0: 0xC03C}
    LAYOUT = STATUS_LAYOUT

    def __init__(self):
        super().__init__("Status", Status.LAYOUT,
                         Status.BASE_ADDRESSES)


class PWMEncoder(Module):
    BASE_ADDRESSES = {0: 0xCB00, 1: 0xCB08, 2: 0xCB10,
                      3: 0xCB18, 4: 0xCB20, 5: 0xCB28, 6: 0xCB30, 7: 0xCB38}
    LAYOUT = PWM_ENCODER_LAYOUT

    def __init__(self):
        super().__init__("PWM_ENCODER", PWMEncoder.LAYOUT,
                         PWMEncoder.BASE_ADDRESSES)


class PWMDecoder(Module):
    BASE_ADDRESSES = {0: 0xCB40, 1: 0xCB48, 2: 0xCB50, 3: 0xCB58, 4: 0xCB60, 5: 0xCB68, 6: 0xCB70, 7: 0xCB80,
                      8: 0xCB88, 9: 0xCB90, 10: 0xCB98, 11: 0xCBA0, 12: 0xCBA8, 13: 0xCBB0, 14: 0xCBB8, 15: 0xCBC0}
    LAYOUT = PWM_DECODER_LAYOUT

    def __init__(self):
        super().__init__("PWM_DECODER", PWMDecoder.LAYOUT,
                         PWMDecoder.BASE_ADDRESSES)


class TOD(Module):
    BASE_ADDRESSES = {0: 0xCBC8, 1: 0xCBCC, 2: 0xCBD0, 3: 0xCBD2}
    LAYOUT = TOD_LAYOUT

    def __init__(self):
        super().__init__("TOD", TOD.LAYOUT, TOD.BASE_ADDRESSES)


class TODWrite(Module):
    BASE_ADDRESSES = {0: 0xCC00, 1: 0xCC10, 2: 0xCC20, 3: 0xCC30}
    LAYOUT = TOD_WRITE_LAYOUT

    def __init__(self):
        super().__init__("TOD_WRITE", TODWrite.LAYOUT,
                         TODWrite.BASE_ADDRESSES)


class TODReadPrimary(Module):
    BASE_ADDRESSES = {0: 0xCC40, 1: 0xCC50, 2: 0xCC60, 3: 0xCC80}
    LAYOUT = TOD_READ_PRIMARY_LAYOUT

    def __init__(self):
        super().__init__("TOD_READ_PRIMARY", TODReadPrimary.LAYOUT,
                         TODReadPrimary.BASE_ADDRESSES)


class TODReadSecondary(Module):
    BASE_ADDRESSES = {0: 0xCC90, 1: 0xCCA0, 2: 0xCCB0, 3: 0xCCC0}
    LAYOUT = TOD_READ_SECONDARY_LAYOUT

    def __init__(self):
        super().__init__("TOD_READ_SECONDARY", TODReadSecondary.LAYOUT,
                         TODReadSecondary.BASE_ADDRESSES)


class Input(Module):
    BASE_ADDRESSES = {0: 0xC1B0, 1: 0xC1C0, 2: 0xC1D0, 3: 0xC200, 4: 0xC210, 5: 0xC220, 6: 0xC230, 7: 0xC240,
                      8: 0xC250, 9: 0xC260, 10: 0xC280, 11: 0xC290, 12: 0xC2A0, 13: 0xC2B0, 14: 0xC2C0, 15: 0xC2D0}
    LAYOUT = INPUT_LAYOUT

    def __init__(self):
        super().__init__("INPUT", Input.LAYOUT, Input.BASE_ADDRESSES)


class Output(Module):
    BASE_ADDRESSES = {0: 0xCA14, 1: 0xCA24, 2: 0xCA34, 3: 0xCA44, 4: 0xca54,
            5:0xca64, 6:0xca80, 7:0xca90, 8:0xcaa0, 9:0xcab0, 10:0xcac0, 11:0xcad0}
    LAYOUT = OUTPUT_LAYOUT

    def __init__(self):
        super().__init__("OUTPUT", Output.LAYOUT, Output.BASE_ADDRESSES)

class REFMON(Module):
    BASE_ADDRESSES = {0: 0xC2E0, 1: 0xC2EC, 2: 0xC300, 3: 0xC30C, 4: 0xC318, 5: 0xC324, 6: 0xC330, 7: 0xC33C,
                      8: 0xC348, 9: 0xC354, 10: 0xC360, 11: 0xC36C, 12: 0xC380, 13: 0xC38C, 14: 0xC398, 15: 0xC3A4}
    LAYOUT = REF_MON_LAYOUT

    def __init__(self):
        super().__init__("REF_MON", REFMON.LAYOUT,
                         REFMON.BASE_ADDRESSES)


class PWM_USER_DATA(Module):
    BASE_ADDRESSES = {0: 0xCBC8}  # Only one base address for PWM_USER_DATA
    LAYOUT = PWM_USER_DATA_LAYOUT

    def __init__(self):
        super().__init__("PWM_USER_DATA", PWM_USER_DATA.LAYOUT,
                         PWM_USER_DATA.BASE_ADDRESSES)


class EEPROM(Module):
    BASE_ADDRESSES = {0: 0xCF68}  # Only one base address for PWM_USER_DATA
    LAYOUT = EEPROM_LAYOUT

    def __init__(self):
        super().__init__("EEPROM", EEPROM.LAYOUT,
                         EEPROM.BASE_ADDRESSES)


class EEPROM_DATA(Module):
    BASE_ADDRESSES = {0: 0xCF80}  # Only one base address for PWM_USER_DATA
    LAYOUT = BYTE_BUFFER_LAYOUT

    def __init__(self):
        super().__init__("EEPROM_DATA", EEPROM_DATA.LAYOUT,
                         EEPROM_DATA.BASE_ADDRESSES)


class OUTPUT_TDC_CFG(Module):
    BASE_ADDRESSES = {0: 0xCCD0}  # Example of multiple base addresses
    LAYOUT = OUTPUT_TDC_CFG_LAYOUT

    def __init__(self):
        super().__init__("OUTPUT_TDC_CFG", OUTPUT_TDC_CFG.LAYOUT,
                         OUTPUT_TDC_CFG.BASE_ADDRESSES)


class OUTPUT_TDC(Module):
    BASE_ADDRESSES = {0: 0xCD00, 1: 0xCD08, 2: 0xCD10,
                      3: 0xCD18}  # Example of multiple base addresses
    LAYOUT = OUTPUT_TDC_LAYOUT

    def __init__(self):
        super().__init__("OUTPUT_TDC", OUTPUT_TDC.LAYOUT,
                         OUTPUT_TDC.BASE_ADDRESSES)


class INPUT_TDC(Module):
    BASE_ADDRESSES = {0: 0xCD20}  # Only one base address for INPUT_TDC
    LAYOUT = INPUT_TDC_LAYOUT

    def __init__(self):
        super().__init__("INPUT_TDC", INPUT_TDC.LAYOUT,
                         INPUT_TDC.BASE_ADDRESSES)


class PWM_SYNC_ENCODER(Module):
    BASE_ADDRESSES = {0: 0xCD80, 1: 0xCD84, 2: 0xCD88, 3: 0xCD8C, 4: 0xCD90,
                      5: 0xCD94, 6: 0xCD98, 7: 0xCD9C}  # Example of multiple base addresses
    LAYOUT = PWM_SYNC_ENCODER_LAYOUT

    def __init__(self):
        super().__init__("PWM_SYNC_ENCODER", PWM_SYNC_ENCODER.LAYOUT,
                         PWM_SYNC_ENCODER.BASE_ADDRESSES)


class PWM_SYNC_DECODER(Module):
    BASE_ADDRESSES = {
        0: 0xCE00, 1: 0xCE06, 2: 0xCE0C, 3: 0xCE12,
        4: 0xCE18, 5: 0xCE1E, 6: 0xCE24, 7: 0xCE2A,
        8: 0xCE30, 9: 0xCE36, 10: 0xCE3C, 11: 0xCE42,
        12: 0xCE48, 13: 0xCE4E, 14: 0xCE54, 15: 0xCE5A
    }
    LAYOUT = PWM_SYNC_DECODER_LAYOUT

    def __init__(self):
        super().__init__("PWM_SYNC_DECODER", PWM_SYNC_DECODER.LAYOUT,
                         PWM_SYNC_DECODER.BASE_ADDRESSES)


class PWM_Rx_Info(Module):
    BASE_ADDRESSES = {0: 0xce80}
    LAYOUT = PWM_RX_INFO_LAYOUT

    def __init__(self):
        super().__init__("PWM_Rx_Info", PWM_Rx_Info.LAYOUT,
                         PWM_Rx_Info.BASE_ADDRESSES)


class DPLL_Ctrl(Module):
    BASE_ADDRESSES = {0: 0xc600, 1: 0xc63c, 2: 0xc680, 3: 0xc6bc,
            4: 0xc700, 5: 0xc73c, 6: 0xc780, 7: 0xc7bc}
    LAYOUT = DPLL_CTRL_LAYOUT

    def __init__(self):
        super().__init__("DPLL_Ctrl", DPLL_Ctrl.LAYOUT,
                         DPLL_Ctrl.BASE_ADDRESSES)


class DPLL_Freq_Write(Module):
    BASE_ADDRESSES = {0: 0xc838, 1: 0xc840, 2: 0xc848, 3: 0xc850,
            4: 0xc858, 5: 0xc860, 6: 0xc868, 7: 0xc870}
    LAYOUT = DPLL_FREQ_WRITE_LAYOUT

    def __init__(self):
        super().__init__("DPLL_Freq_Write", DPLL_Freq_Write.LAYOUT,
                         DPLL_Freq_Write.BASE_ADDRESSES)


class DPLL_Config(Module):
    BASE_ADDRESSES = {0: 0xc3b0, 1: 0xc400, 2: 0xc438, 3: 0xc480,
            4: 0xc4b8, 5: 0xc500, 6: 0xc538, 7: 0xc580}
    LAYOUT = DPLL_LAYOUT

    def __init__(self):
        super().__init__("DPLL_Config", DPLL_Config.LAYOUT,
                         DPLL_Config.BASE_ADDRESSES)



class DPLL_GeneralStatus(Module):
    BASE_ADDRESSES = {0: 0xc014}
    LAYOUT = DPLL_GENERAL_STATUS_LAYOUT

    def __init__(self):
        super().__init__("DPLL_GeneralStatus", DPLL_GeneralStatus.LAYOUT,
                         DPLL_GeneralStatus.BASE_ADDRESSES)
# holder of registers
class DPLL():
    def __init__(self, i2c_dev,
                 read_func, read_mul_func, write_func, write_mul_func):
        # make modules inside the dpll
        self.modules = {}

        modules_to_use = [Status, PWMEncoder, PWMDecoder, TOD, TODWrite, TODReadPrimary,
                          TODReadSecondary, Input, Output, REFMON, PWM_USER_DATA,
                          OUTPUT_TDC_CFG, OUTPUT_TDC, INPUT_TDC, PWM_SYNC_ENCODER,
                          PWM_SYNC_DECODER, EEPROM, EEPROM_DATA, PWM_Rx_Info, DPLL_Ctrl,
                          DPLL_Freq_Write, DPLL_Config, DPLL_GeneralStatus]

        for mod in modules_to_use:
            self.modules[mod.__name__] = mod()
            self.modules[mod.__name__].read_func = read_func
            self.modules[mod.__name__].read_mul_func = read_mul_func
            self.modules[mod.__name__].write_func = write_func
            self.modules[mod.__name__].write_mul_func = write_mul_func
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
