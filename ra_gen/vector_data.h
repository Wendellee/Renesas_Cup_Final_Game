/* generated vector header file - do not edit */
#ifndef VECTOR_DATA_H
#define VECTOR_DATA_H
#ifdef __cplusplus
        extern "C" {
        #endif
/* Number of interrupts allocated */
#ifndef VECTOR_DATA_IRQ_COUNT
#define VECTOR_DATA_IRQ_COUNT    (20)
#endif
/* ISR prototypes */
void spi_b_rxi_isr(void);
void spi_b_txi_isr(void);
void spi_b_tei_isr(void);
void spi_b_eri_isr(void);
void iic_master_rxi_isr(void);
void iic_master_txi_isr(void);
void iic_master_tei_isr(void);
void iic_master_eri_isr(void);
void glcdc_line_detect_isr(void);
void drw_int_isr(void);
void r_icu_isr(void);
void vin_status_isr(void);
void mipi_csi_rx_isr(void);
void mipi_csi_dl_isr(void);
void mipi_csi_vc_isr(void);

/* Vector table allocations */
#define VECTOR_NUMBER_SPI0_RXI ((IRQn_Type) 0) /* SPI0 RXI (Receive buffer full) */
#define SPI0_RXI_IRQn          ((IRQn_Type) 0) /* SPI0 RXI (Receive buffer full) */
#define VECTOR_NUMBER_SPI0_TXI ((IRQn_Type) 1) /* SPI0 TXI (Transmit buffer empty) */
#define SPI0_TXI_IRQn          ((IRQn_Type) 1) /* SPI0 TXI (Transmit buffer empty) */
#define VECTOR_NUMBER_SPI0_TEI ((IRQn_Type) 2) /* SPI0 TEI (Transmission complete event) */
#define SPI0_TEI_IRQn          ((IRQn_Type) 2) /* SPI0 TEI (Transmission complete event) */
#define VECTOR_NUMBER_SPI0_ERI ((IRQn_Type) 3) /* SPI0 ERI (Error) */
#define SPI0_ERI_IRQn          ((IRQn_Type) 3) /* SPI0 ERI (Error) */
#define VECTOR_NUMBER_SPI1_RXI ((IRQn_Type) 4) /* SPI1 RXI (Receive buffer full) */
#define SPI1_RXI_IRQn          ((IRQn_Type) 4) /* SPI1 RXI (Receive buffer full) */
#define VECTOR_NUMBER_SPI1_TXI ((IRQn_Type) 5) /* SPI1 TXI (Transmit buffer empty) */
#define SPI1_TXI_IRQn          ((IRQn_Type) 5) /* SPI1 TXI (Transmit buffer empty) */
#define VECTOR_NUMBER_SPI1_TEI ((IRQn_Type) 6) /* SPI1 TEI (Transmission complete event) */
#define SPI1_TEI_IRQn          ((IRQn_Type) 6) /* SPI1 TEI (Transmission complete event) */
#define VECTOR_NUMBER_SPI1_ERI ((IRQn_Type) 7) /* SPI1 ERI (Error) */
#define SPI1_ERI_IRQn          ((IRQn_Type) 7) /* SPI1 ERI (Error) */
#define VECTOR_NUMBER_IIC1_RXI ((IRQn_Type) 8) /* IIC1 RXI (Receive data full) */
#define IIC1_RXI_IRQn          ((IRQn_Type) 8) /* IIC1 RXI (Receive data full) */
#define VECTOR_NUMBER_IIC1_TXI ((IRQn_Type) 9) /* IIC1 TXI (Transmit data empty) */
#define IIC1_TXI_IRQn          ((IRQn_Type) 9) /* IIC1 TXI (Transmit data empty) */
#define VECTOR_NUMBER_IIC1_TEI ((IRQn_Type) 10) /* IIC1 TEI (Transmit end) */
#define IIC1_TEI_IRQn          ((IRQn_Type) 10) /* IIC1 TEI (Transmit end) */
#define VECTOR_NUMBER_IIC1_ERI ((IRQn_Type) 11) /* IIC1 ERI (Transfer error) */
#define IIC1_ERI_IRQn          ((IRQn_Type) 11) /* IIC1 ERI (Transfer error) */
#define VECTOR_NUMBER_GLCDC_LINE_DETECT ((IRQn_Type) 12) /* GLCDC LINE DETECT (Specified line) */
#define GLCDC_LINE_DETECT_IRQn          ((IRQn_Type) 12) /* GLCDC LINE DETECT (Specified line) */
#define VECTOR_NUMBER_DRW_INT ((IRQn_Type) 13) /* DRW INT (DRW interrupt) */
#define DRW_INT_IRQn          ((IRQn_Type) 13) /* DRW INT (DRW interrupt) */
#define VECTOR_NUMBER_ICU_IRQ19 ((IRQn_Type) 14) /* ICU IRQ19 (External pin interrupt 19) */
#define ICU_IRQ19_IRQn          ((IRQn_Type) 14) /* ICU IRQ19 (External pin interrupt 19) */
#define VECTOR_NUMBER_ICU_IRQ0 ((IRQn_Type) 15) /* ICU IRQ0 (External pin interrupt 0) */
#define ICU_IRQ0_IRQn          ((IRQn_Type) 15) /* ICU IRQ0 (External pin interrupt 0) */
#define VECTOR_NUMBER_VIN_IRQ ((IRQn_Type) 16) /* VIN IRQ (Interrupt Request) */
#define VIN_IRQ_IRQn          ((IRQn_Type) 16) /* VIN IRQ (Interrupt Request) */
#define VECTOR_NUMBER_MIPICSI_RX ((IRQn_Type) 17) /* MIPICSI RX (Receive interrupt) */
#define MIPICSI_RX_IRQn          ((IRQn_Type) 17) /* MIPICSI RX (Receive interrupt) */
#define VECTOR_NUMBER_MIPICSI_DL ((IRQn_Type) 18) /* MIPICSI DL (Data Lane interrupt) */
#define MIPICSI_DL_IRQn          ((IRQn_Type) 18) /* MIPICSI DL (Data Lane interrupt) */
#define VECTOR_NUMBER_MIPICSI_VC ((IRQn_Type) 19) /* MIPICSI VC (Virtual Channel interrupt) */
#define MIPICSI_VC_IRQn          ((IRQn_Type) 19) /* MIPICSI VC (Virtual Channel interrupt) */
/* The number of entries required for the ICU vector table. */
#define BSP_ICU_VECTOR_NUM_ENTRIES (20)

#ifdef __cplusplus
        }
        #endif
#endif /* VECTOR_DATA_H */
