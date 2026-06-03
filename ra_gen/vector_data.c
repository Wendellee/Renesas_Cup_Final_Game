/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = iic_master_rxi_isr, /* IIC1 RXI (Receive data full) */
            [1] = iic_master_txi_isr, /* IIC1 TXI (Transmit data empty) */
            [2] = iic_master_tei_isr, /* IIC1 TEI (Transmit end) */
            [3] = iic_master_eri_isr, /* IIC1 ERI (Transfer error) */
            [4] = vin_status_isr, /* VIN IRQ (Interrupt Request) */
            [5] = mipi_csi_rx_isr, /* MIPICSI RX (Receive interrupt) */
            [6] = mipi_csi_dl_isr, /* MIPICSI DL (Data Lane interrupt) */
            [7] = mipi_csi_vc_isr, /* MIPICSI VC (Virtual Channel interrupt) */
            [8] = glcdc_line_detect_isr, /* GLCDC LINE DETECT (Specified line) */
            [9] = glcdc_underflow_1_isr, /* GLCDC UNDERFLOW 1 (Graphic 1 underflow) */
            [10] = glcdc_underflow_2_isr, /* GLCDC UNDERFLOW 2 (Graphic 2 underflow) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_IIC1_RXI,GROUP0), /* IIC1 RXI (Receive data full) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_IIC1_TXI,GROUP1), /* IIC1 TXI (Transmit data empty) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_IIC1_TEI,GROUP2), /* IIC1 TEI (Transmit end) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_IIC1_ERI,GROUP3), /* IIC1 ERI (Transfer error) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_VIN_IRQ,GROUP4), /* VIN IRQ (Interrupt Request) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_RX,GROUP5), /* MIPICSI RX (Receive interrupt) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_DL,GROUP6), /* MIPICSI DL (Data Lane interrupt) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_VC,GROUP7), /* MIPICSI VC (Virtual Channel interrupt) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_GLCDC_LINE_DETECT,GROUP0), /* GLCDC LINE DETECT (Specified line) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_GLCDC_UNDERFLOW_1,GROUP1), /* GLCDC UNDERFLOW 1 (Graphic 1 underflow) */
            [10] = BSP_PRV_VECT_ENUM(EVENT_GLCDC_UNDERFLOW_2,GROUP2), /* GLCDC UNDERFLOW 2 (Graphic 2 underflow) */
        };
        #endif
        #endif
