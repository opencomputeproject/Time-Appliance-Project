/*
 * nettimelogic.h
 *
 *  Created on: 19.10.2020
 *      Author: thomas
 */

#ifndef SRC_NETTIMELOGIC_H_
#define SRC_NETTIMELOGIC_H_

/*****************************************************************************/
/* Register definitions                                                      */
/*****************************************************************************/
/* Register Set */
#define NTL_CLK_REGSET_BASE                             0x01000000

#define NTL_CLK_CLKCONTROL_REG                          0x00000000
#define NTL_CLK_CLKSTATUS_REG                           0x00000004
#define NTL_CLK_CLKSELECT_REG                           0x00000008
#define NTL_CLK_CLKVERSION_REG                          0x0000000C
#define NTL_CLK_CLKTIMEVALUEL_REG                       0x00000010
#define NTL_CLK_CLKTIMEVALUEH_REG                       0x00000014
#define NTL_CLK_CLKTIMEADJVALUEL_REG                    0x00000020
#define NTL_CLK_CLKTIMEADJVALUEH_REG                    0x00000024
#define NTL_CLK_CLKOFFSETADJVALUE_REG                   0x00000030
#define NTL_CLK_CLKOFFSETADJINTERVAL_REG                0x00000034
#define NTL_CLK_CLKDRIFTADJVALUE_REG                    0x00000040
#define NTL_CLK_CLKDRIFTADJINTERVAL_REG                 0x00000044
#define NTL_CLK_CLKINSYNCTHRESHOLD_REG                  0x00000050
#define NTL_CLK_CLKSERVOOFFSETFACTORP_REG               0x00000060
#define NTL_CLK_CLKSERVOOFFSETFACTORI_REG               0x00000064
#define NTL_CLK_CLKSERVODRIFTFACTORP_REG                0x00000068
#define NTL_CLK_CLKSERVODRIFTFACTORI_REG                0x0000006C
#define NTL_CLK_CLKSTATUSOFFSET_REG                     0x00000070
#define NTL_CLK_CLKSTATUSDRIFT_REG                      0x00000074

/*****************************************************************************/
/* Register definitions                                                      */
/*****************************************************************************/
#define NTL_CLK_CONTROL_ENABLE_BIT                      0x00000001
#define NTL_CLK_CONTROL_TIME_ADJ_BIT                    0x00000002
#define NTL_CLK_CONTROL_OFFSET_ADJ_BIT                  0x00000004
#define NTL_CLK_CONTROL_DRIFT_ADJ_BIT                   0x00000008
#define NTL_CLK_CONTROL_TIME_BIT                        0x40000000
#define NTL_CLK_CONTROL_TIME_VAL_BIT                    0x80000000
            
#define NTL_CLK_STATUS_IN_SYNC_BIT                      0x00000001
            
#define NTL_CLK_SELECT_NONE                             0
#define NTL_CLK_SELECT_TOD                              1
#define NTL_CLK_SELECT_IRIG                             2
#define NTL_CLK_SELECT_PPS                              3
#define NTL_CLK_SELECT_PTP                              4 
#define NTL_CLK_SELECT_RTC                              5
#define NTL_CLK_SELECT_DCF                              6
#define NTL_CLK_SELECT_REGS                             254    
#define NTL_CLK_SELECT_EXT                              255
        
/*****************************************************************************/
/* Register definitions                                                      */
/*****************************************************************************/
/* Register Set */
#define NTL_STS1_REGSET_BASE                            0x01010000
#define NTL_STS2_REGSET_BASE                            0x01020000

#define NTL_STS_CONTROL_REG                             0x00000000
#define NTL_STS_STATUS_REG                              0x00000004
#define NTL_STS_POLARITY_REG                            0x00000008
#define NTL_STS_VERSION_REG                             0x0000000C
#define NTL_STS_CABLEDELAY_REG                          0x00000020
#define NTL_STS_IRQ_REG                                 0x00000030
#define NTL_STS_IRQMASK_REG                             0x00000034
#define NTL_STS_EVTCOUNT_REG                            0x00000038
#define NTL_STS_COUNT_REG                               0x00000040
#define NTL_STS_TIMEVALUEL_REG                          0x00000044
#define NTL_STS_TIMEVALUEH_REG                          0x00000048
#define NTL_STS_DATAWIDTH_REG                           0x0000004C
#define NTL_STS_DATA_REG                                0x00000050

/*****************************************************************************/
/* Register definitions                                                      */
/*****************************************************************************/
#define NTL_STS_CONTROL_ENABLE_BIT                      0x00000001
#define NTL_STS_STATUS_DROP_BIT                         0x00000001
#define NTL_STS_POLARITY_BIT                            0x00000001
#define NTL_STS_IRQ_VALID_BIT                           0x00000001
#define NTL_STS_IRQMASK_VALID_BIT                       0x00000001

/*****************************************************************************/
/* Register definitions                                                      */
/*****************************************************************************/
/* Register Set */
#define NTL_PPSM_REGSET_BASE                            0x01030000

#define NTL_PPSM_CONTROL_REG                            0x00000000
#define NTL_PPSM_STATUS_REG                             0x00000004
#define NTL_PPSM_POLARITY_REG                           0x00000008
#define NTL_PPSM_VERSION_REG                            0x0000000C
#define NTL_PPSM_PULSEWIDTH_REG                         0x00000010
#define NTL_PPSM_CABLEDELAY_REG                         0x00000020

/*****************************************************************************/
/* Register definitions                                                      */
/*****************************************************************************/
#define NTL_PPSM_CONTROL_ENABLE_BIT                     0x00000001
#define NTL_PPSM_STATUS_ERROR_BIT                       0x00000001
#define NTL_PPSM_POLARITY_BIT                           0x00000001

/*****************************************************************************/
/* Register definitions                                                      */
/*****************************************************************************/
/* Register Set */
#define NTL_PPSS_REGSET_BASE                            0x01040000

#define NTL_PPSS_CONTROL_REG                            0x00000000
#define NTL_PPSS_STATUS_REG                             0x00000004
#define NTL_PPSS_POLARITY_REG                           0x00000008
#define NTL_PPSS_VERSION_REG                            0x0000000C
#define NTL_PPSS_PULSEWIDTH_REG                         0x00000010
#define NTL_PPSS_CABLEDELAY_REG                         0x00000020

/*****************************************************************************/
/* Register definitions                                                      */
/*****************************************************************************/
#define NTL_PPSS_CONTROL_ENABLE_BIT                     0x00000001
#define NTL_PPSS_STATUS_FILTER_ERROR_BIT                0x00000001
#define NTL_PPSS_STATUS_SUPERVISION_ERROR_BIT           0x00000002
#define NTL_PPSS_STATUS_ERROR_BIT                       0x00000001
#define NTL_PPSS_POLARITY_BIT                           0x00000001

/*****************************************************************************/
/* Register definitions                                                      */
/*****************************************************************************/
/* Register Set */
#define NTL_TOD_REGSET_BASE                             0x01050000

#define NTL_TOD_CONTROL_REG                             0x00000000
#define NTL_TOD_STATUS_REG                              0x00000004
#define NTL_TOD_POLARITY_REG                            0x00000008
#define NTL_TOD_VERSION_REG                             0x0000000C
#define NTL_TOD_CORRECTION_REG                          0x00000010
#define NTL_TOD_BAUDRATE_REG                            0x00000020

/*****************************************************************************/
/* Register definitions                                                      */
/*****************************************************************************/
#define NTL_TOD_CONTROL_ENABLE_BIT                      0x00000001
#define NTL_TOD_STATUS_RROR_BIT                         0x00000001
     
#endif /* SRC_NETTIMELOGIC_H_ */
