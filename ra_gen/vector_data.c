/* generated vector source file - do not edit */
        #include "bsp_api.h"
        /* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
        #if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = spi_b_rxi_isr, /* SPI0 RXI (Receive buffer full) */
            [1] = spi_b_txi_isr, /* SPI0 TXI (Transmit buffer empty) */
            [2] = spi_b_tei_isr, /* SPI0 TEI (Transmission complete event) */
            [3] = spi_b_eri_isr, /* SPI0 ERI (Error) */
            [4] = spi_b_rxi_isr, /* SPI1 RXI (Receive buffer full) */
            [5] = spi_b_txi_isr, /* SPI1 TXI (Transmit buffer empty) */
            [6] = spi_b_tei_isr, /* SPI1 TEI (Transmission complete event) */
            [7] = spi_b_eri_isr, /* SPI1 ERI (Error) */
            [8] = iic_master_rxi_isr, /* IIC1 RXI (Receive data full) */
            [9] = iic_master_txi_isr, /* IIC1 TXI (Transmit data empty) */
            [10] = iic_master_tei_isr, /* IIC1 TEI (Transmit end) */
            [11] = iic_master_eri_isr, /* IIC1 ERI (Transfer error) */
            [12] = glcdc_line_detect_isr, /* GLCDC LINE DETECT (Specified line) */
            [13] = drw_int_isr, /* DRW INT (DRW interrupt) */
            [14] = r_icu_isr, /* ICU IRQ19 (External pin interrupt 19) */
            [15] = r_icu_isr, /* ICU IRQ0 (External pin interrupt 0) */
            [16] = vin_status_isr, /* VIN IRQ (Interrupt Request) */
            [17] = mipi_csi_rx_isr, /* MIPICSI RX (Receive interrupt) */
            [18] = mipi_csi_dl_isr, /* MIPICSI DL (Data Lane interrupt) */
            [19] = mipi_csi_vc_isr, /* MIPICSI VC (Virtual Channel interrupt) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_SPI0_RXI,GROUP0), /* SPI0 RXI (Receive buffer full) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_SPI0_TXI,GROUP1), /* SPI0 TXI (Transmit buffer empty) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_SPI0_TEI,GROUP2), /* SPI0 TEI (Transmission complete event) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_SPI0_ERI,GROUP3), /* SPI0 ERI (Error) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_SPI1_RXI,GROUP4), /* SPI1 RXI (Receive buffer full) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_SPI1_TXI,GROUP5), /* SPI1 TXI (Transmit buffer empty) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_SPI1_TEI,GROUP6), /* SPI1 TEI (Transmission complete event) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_SPI1_ERI,GROUP7), /* SPI1 ERI (Error) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_IIC1_RXI,GROUP0), /* IIC1 RXI (Receive data full) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_IIC1_TXI,GROUP1), /* IIC1 TXI (Transmit data empty) */
            [10] = BSP_PRV_VECT_ENUM(EVENT_IIC1_TEI,GROUP2), /* IIC1 TEI (Transmit end) */
            [11] = BSP_PRV_VECT_ENUM(EVENT_IIC1_ERI,GROUP3), /* IIC1 ERI (Transfer error) */
            [12] = BSP_PRV_VECT_ENUM(EVENT_GLCDC_LINE_DETECT,GROUP4), /* GLCDC LINE DETECT (Specified line) */
            [13] = BSP_PRV_VECT_ENUM(EVENT_DRW_INT,GROUP5), /* DRW INT (DRW interrupt) */
            [14] = BSP_PRV_VECT_ENUM(EVENT_ICU_IRQ19,GROUP6), /* ICU IRQ19 (External pin interrupt 19) */
            [15] = BSP_PRV_VECT_ENUM(EVENT_ICU_IRQ0,GROUP7), /* ICU IRQ0 (External pin interrupt 0) */
            [16] = BSP_PRV_VECT_ENUM(EVENT_VIN_IRQ,GROUP0), /* VIN IRQ (Interrupt Request) */
            [17] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_RX,GROUP1), /* MIPICSI RX (Receive interrupt) */
            [18] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_DL,GROUP2), /* MIPICSI DL (Data Lane interrupt) */
            [19] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_VC,GROUP3), /* MIPICSI VC (Virtual Channel interrupt) */
        };
        #endif
        #endif