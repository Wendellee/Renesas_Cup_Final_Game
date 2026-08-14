#ifndef NRF24_REGS_H_
#define NRF24_REGS_H_

/* nRF24L01+ 的 SPI 命令定义 */
#define NRF24_CMD_R_REGISTER          (0x00U)
#define NRF24_CMD_W_REGISTER          (0x20U)
#define NRF24_CMD_R_RX_PAYLOAD        (0x61U)
#define NRF24_CMD_R_RX_PL_WID         (0x60U)
#define NRF24_CMD_W_TX_PAYLOAD        (0xA0U)
#define NRF24_CMD_W_TX_PAYLOAD_NO_ACK (0xB0U)
#define NRF24_CMD_W_ACK_PAYLOAD       (0xA8U)
#define NRF24_CMD_FLUSH_TX            (0xE1U)
#define NRF24_CMD_FLUSH_RX            (0xE2U)
#define NRF24_CMD_REUSE_TX_PL         (0xE3U)
#define NRF24_CMD_ACTIVATE            (0x50U)
#define NRF24_CMD_NOP                 (0xFFU)

/* nRF24L01+ 的寄存器地址定义 */
#define NRF24_REG_CONFIG              (0x00U)
#define NRF24_REG_EN_AA               (0x01U)
#define NRF24_REG_EN_RXADDR           (0x02U)
#define NRF24_REG_SETUP_AW            (0x03U)
#define NRF24_REG_SETUP_RETR          (0x04U)
#define NRF24_REG_RF_CH               (0x05U)
#define NRF24_REG_RF_SETUP            (0x06U)
#define NRF24_REG_STATUS              (0x07U)
#define NRF24_REG_OBSERVE_TX          (0x08U)
#define NRF24_REG_RPD                 (0x09U)
#define NRF24_REG_RX_ADDR_P0          (0x0AU)
#define NRF24_REG_RX_ADDR_P1          (0x0BU)
#define NRF24_REG_RX_ADDR_P2          (0x0CU)
#define NRF24_REG_RX_ADDR_P3          (0x0DU)
#define NRF24_REG_RX_ADDR_P4          (0x0EU)
#define NRF24_REG_RX_ADDR_P5          (0x0FU)
#define NRF24_REG_TX_ADDR             (0x10U)
#define NRF24_REG_RX_PW_P0            (0x11U)
#define NRF24_REG_RX_PW_P1            (0x12U)
#define NRF24_REG_RX_PW_P2            (0x13U)
#define NRF24_REG_RX_PW_P3            (0x14U)
#define NRF24_REG_RX_PW_P4            (0x15U)
#define NRF24_REG_RX_PW_P5            (0x16U)
#define NRF24_REG_FIFO_STATUS         (0x17U)
#define NRF24_REG_DYNPD               (0x1CU)
#define NRF24_REG_FEATURE             (0x1DU)

/* 公共掩码与长度限制 */
#define NRF24_REG_MASK                (0x1FU)
#define NRF24_PIPE_COUNT              (6U)
#define NRF24_MAX_PAYLOAD_SIZE        (32U)
#define NRF24_ADDRESS_WIDTH_MIN       (3U)
#define NRF24_ADDRESS_WIDTH_MAX       (5U)
#define NRF24_RF_CHANNEL_MAX          (125U)
#define NRF24_ACTIVATE_DATA           (0x73U)

/* CONFIG 寄存器位定义 */
#define NRF24_CONFIG_MASK_RX_DR       (0x40U)
#define NRF24_CONFIG_MASK_TX_DS       (0x20U)
#define NRF24_CONFIG_MASK_MAX_RT      (0x10U)
#define NRF24_CONFIG_EN_CRC           (0x08U)
#define NRF24_CONFIG_CRCO             (0x04U)
#define NRF24_CONFIG_PWR_UP           (0x02U)
#define NRF24_CONFIG_PRIM_RX          (0x01U)

/* EN_AA、EN_RXADDR 与 DYNPD 的管道位定义 */
#define NRF24_PIPE0_MASK              (0x01U)
#define NRF24_PIPE1_MASK              (0x02U)
#define NRF24_PIPE2_MASK              (0x04U)
#define NRF24_PIPE3_MASK              (0x08U)
#define NRF24_PIPE4_MASK              (0x10U)
#define NRF24_PIPE5_MASK              (0x20U)

/* SETUP_AW 与 SETUP_RETR 寄存器位定义 */
#define NRF24_SETUP_AW_MASK           (0x03U)
#define NRF24_SETUP_RETR_ARD_MASK     (0xF0U)
#define NRF24_SETUP_RETR_ARC_MASK     (0x0FU)

/* RF_SETUP 寄存器位定义 */
#define NRF24_RF_SETUP_CONT_WAVE      (0x80U)
#define NRF24_RF_SETUP_RF_DR_LOW      (0x20U)
#define NRF24_RF_SETUP_PLL_LOCK       (0x10U)
#define NRF24_RF_SETUP_RF_DR_HIGH     (0x08U)
#define NRF24_RF_SETUP_RF_PWR_MASK    (0x06U)
#define NRF24_RF_SETUP_LNA_HCURR      (0x01U)

/* STATUS 寄存器位定义 */
#define NRF24_STATUS_RX_DR            (0x40U)
#define NRF24_STATUS_TX_DS            (0x20U)
#define NRF24_STATUS_MAX_RT           (0x10U)
#define NRF24_STATUS_RX_P_NO_MASK     (0x0EU)
#define NRF24_STATUS_RX_P_NO_SHIFT    (1U)
#define NRF24_STATUS_TX_FULL          (0x01U)
#define NRF24_STATUS_IRQ_MASK         (NRF24_STATUS_RX_DR | NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT)

/* FIFO_STATUS 寄存器位定义 */
#define NRF24_FIFO_STATUS_TX_REUSE    (0x40U)
#define NRF24_FIFO_STATUS_TX_FULL     (0x20U)
#define NRF24_FIFO_STATUS_TX_EMPTY    (0x10U)
#define NRF24_FIFO_STATUS_RX_FULL     (0x02U)
#define NRF24_FIFO_STATUS_RX_EMPTY    (0x01U)

/* FEATURE 寄存器位定义 */
#define NRF24_FEATURE_EN_DPL          (0x04U)
#define NRF24_FEATURE_EN_ACK_PAY      (0x02U)
#define NRF24_FEATURE_EN_DYN_ACK      (0x01U)

#endif

