/* generated vector header file - do not edit */
#ifndef VECTOR_DATA_H
#define VECTOR_DATA_H
#ifdef __cplusplus
        extern "C" {
        #endif
/* Number of interrupts allocated */
#ifndef VECTOR_DATA_IRQ_COUNT
#define VECTOR_DATA_IRQ_COUNT    (14)
#endif
/* ISR prototypes */
void glcdc_line_detect_isr(void);
void glcdc_underflow_1_isr(void);
void glcdc_underflow_2_isr(void);
void drw_int_isr(void);
void iic_master_rxi_isr(void);
void iic_master_txi_isr(void);
void iic_master_tei_isr(void);
void iic_master_eri_isr(void);
void vin_status_isr(void);
void mipi_csi_rx_isr(void);
void mipi_csi_dl_isr(void);
void mipi_csi_vc_isr(void);
void rm_ethosu_isr(void);
void ipc_isr(void);

/* Vector table allocations */
#define VECTOR_NUMBER_GLCDC_LINE_DETECT ((IRQn_Type) 0) /* GLCDC LINE DETECT (Specified line) */
#define GLCDC_LINE_DETECT_IRQn          ((IRQn_Type) 0) /* GLCDC LINE DETECT (Specified line) */
#define VECTOR_NUMBER_GLCDC_UNDERFLOW_1 ((IRQn_Type) 1) /* GLCDC UNDERFLOW 1 (Graphic 1 underflow) */
#define GLCDC_UNDERFLOW_1_IRQn          ((IRQn_Type) 1) /* GLCDC UNDERFLOW 1 (Graphic 1 underflow) */
#define VECTOR_NUMBER_GLCDC_UNDERFLOW_2 ((IRQn_Type) 2) /* GLCDC UNDERFLOW 2 (Graphic 2 underflow) */
#define GLCDC_UNDERFLOW_2_IRQn          ((IRQn_Type) 2) /* GLCDC UNDERFLOW 2 (Graphic 2 underflow) */
#define VECTOR_NUMBER_DRW_INT ((IRQn_Type) 3) /* DRW INT (DRW interrupt) */
#define DRW_INT_IRQn          ((IRQn_Type) 3) /* DRW INT (DRW interrupt) */
#define VECTOR_NUMBER_IIC1_RXI ((IRQn_Type) 4) /* IIC1 RXI (Receive data full) */
#define IIC1_RXI_IRQn          ((IRQn_Type) 4) /* IIC1 RXI (Receive data full) */
#define VECTOR_NUMBER_IIC1_TXI ((IRQn_Type) 5) /* IIC1 TXI (Transmit data empty) */
#define IIC1_TXI_IRQn          ((IRQn_Type) 5) /* IIC1 TXI (Transmit data empty) */
#define VECTOR_NUMBER_IIC1_TEI ((IRQn_Type) 6) /* IIC1 TEI (Transmit end) */
#define IIC1_TEI_IRQn          ((IRQn_Type) 6) /* IIC1 TEI (Transmit end) */
#define VECTOR_NUMBER_IIC1_ERI ((IRQn_Type) 7) /* IIC1 ERI (Transfer error) */
#define IIC1_ERI_IRQn          ((IRQn_Type) 7) /* IIC1 ERI (Transfer error) */
#define VECTOR_NUMBER_VIN_IRQ ((IRQn_Type) 8) /* VIN IRQ (Interrupt Request) */
#define VIN_IRQ_IRQn          ((IRQn_Type) 8) /* VIN IRQ (Interrupt Request) */
#define VECTOR_NUMBER_MIPICSI_RX ((IRQn_Type) 9) /* MIPICSI RX (Receive interrupt) */
#define MIPICSI_RX_IRQn          ((IRQn_Type) 9) /* MIPICSI RX (Receive interrupt) */
#define VECTOR_NUMBER_MIPICSI_DL ((IRQn_Type) 10) /* MIPICSI DL (Data Lane interrupt) */
#define MIPICSI_DL_IRQn          ((IRQn_Type) 10) /* MIPICSI DL (Data Lane interrupt) */
#define VECTOR_NUMBER_MIPICSI_VC ((IRQn_Type) 11) /* MIPICSI VC (Virtual Channel interrupt) */
#define MIPICSI_VC_IRQn          ((IRQn_Type) 11) /* MIPICSI VC (Virtual Channel interrupt) */
#define VECTOR_NUMBER_NPU_IRQ ((IRQn_Type) 12) /* NPU IRQ (NPU IRQ) */
#define NPU_IRQ_IRQn          ((IRQn_Type) 12) /* NPU IRQ (NPU IRQ) */
#define VECTOR_NUMBER_IPC_IRQ0 ((IRQn_Type) 13) /* IPC IRQ0 (CPU Mutual Interrupt 0) */
#define IPC_IRQ0_IRQn          ((IRQn_Type) 13) /* IPC IRQ0 (CPU Mutual Interrupt 0) */
/* The number of entries required for the ICU vector table. */
#define BSP_ICU_VECTOR_NUM_ENTRIES (14)

#ifdef __cplusplus
        }
        #endif
#endif /* VECTOR_DATA_H */
