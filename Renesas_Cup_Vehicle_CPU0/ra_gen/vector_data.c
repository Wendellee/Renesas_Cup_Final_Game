/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = glcdc_line_detect_isr, /* GLCDC LINE DETECT (Specified line) */
            [1] = glcdc_underflow_1_isr, /* GLCDC UNDERFLOW 1 (Graphic 1 underflow) */
            [2] = glcdc_underflow_2_isr, /* GLCDC UNDERFLOW 2 (Graphic 2 underflow) */
            [3] = drw_int_isr, /* DRW INT (DRW interrupt) */
            [4] = iic_master_rxi_isr, /* IIC1 RXI (Receive data full) */
            [5] = iic_master_txi_isr, /* IIC1 TXI (Transmit data empty) */
            [6] = iic_master_tei_isr, /* IIC1 TEI (Transmit end) */
            [7] = iic_master_eri_isr, /* IIC1 ERI (Transfer error) */
            [8] = vin_status_isr, /* VIN IRQ (Interrupt Request) */
            [9] = mipi_csi_rx_isr, /* MIPICSI RX (Receive interrupt) */
            [10] = mipi_csi_dl_isr, /* MIPICSI DL (Data Lane interrupt) */
            [11] = mipi_csi_vc_isr, /* MIPICSI VC (Virtual Channel interrupt) */
            [12] = rm_ethosu_isr, /* NPU IRQ (NPU IRQ) */
            [13] = ipc_isr, /* IPC IRQ0 (CPU Mutual Interrupt 0) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_GLCDC_LINE_DETECT,GROUP0), /* GLCDC LINE DETECT (Specified line) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_GLCDC_UNDERFLOW_1,GROUP1), /* GLCDC UNDERFLOW 1 (Graphic 1 underflow) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_GLCDC_UNDERFLOW_2,GROUP2), /* GLCDC UNDERFLOW 2 (Graphic 2 underflow) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_DRW_INT,GROUP3), /* DRW INT (DRW interrupt) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_IIC1_RXI,GROUP4), /* IIC1 RXI (Receive data full) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_IIC1_TXI,GROUP5), /* IIC1 TXI (Transmit data empty) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_IIC1_TEI,GROUP6), /* IIC1 TEI (Transmit end) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_IIC1_ERI,GROUP7), /* IIC1 ERI (Transfer error) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_VIN_IRQ,GROUP0), /* VIN IRQ (Interrupt Request) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_RX,GROUP1), /* MIPICSI RX (Receive interrupt) */
            [10] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_DL,GROUP2), /* MIPICSI DL (Data Lane interrupt) */
            [11] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_VC,GROUP3), /* MIPICSI VC (Virtual Channel interrupt) */
            [12] = BSP_PRV_VECT_ENUM(EVENT_NPU_IRQ,GROUP4), /* NPU IRQ (NPU IRQ) */
            [13] = BSP_PRV_VECT_ENUM(EVENT_IPC_IRQ0,GROUP5), /* IPC IRQ0 (CPU Mutual Interrupt 0) */
        };
        #endif
        #endif
