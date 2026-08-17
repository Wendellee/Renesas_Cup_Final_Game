#include "IPC/shared_jpeg_protocol.h"

/*
 *[@name] shared_jpeg_crc32
 *[@type] function
 *[@usage] 使用反射多项式0xEDB88320逐字节计算CRC-32/ISO-HDLC
 *[@argument] p_data 待校验数据的只读首地址
 *[@argument] length 待校验数据长度，单位为字节
 *[@return] 返回32位CRC；参数无效时返回0
 */
uint32_t shared_jpeg_crc32(const uint8_t * p_data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;

    if((NULL == p_data) || (0U == length))
    {
        return 0U;
    }

    for(size_t byte_index = 0U; byte_index < length; byte_index++)
    {
        crc ^= (uint32_t) p_data[byte_index];

        for(uint32_t bit_index = 0U; bit_index < 8U; bit_index++)
        {
            if(0U != (crc & 1UL))
            {
                crc = (crc >> 1U) ^ 0xEDB88320UL;
            }
            else
            {
                crc >>= 1U;
            }
        }
    }

    return crc ^ 0xFFFFFFFFUL;
}
