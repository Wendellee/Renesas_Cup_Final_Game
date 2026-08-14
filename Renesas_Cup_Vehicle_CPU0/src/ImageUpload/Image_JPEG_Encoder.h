#ifndef IMAGE_UPLOAD_IMAGE_JPEG_ENCODER_H_
#define IMAGE_UPLOAD_IMAGE_JPEG_ENCODER_H_

#include "hal_data.h"
#include <stddef.h>
#include <stdint.h>

typedef struct st_image_jpeg_encode_cfg
{
    uint16_t source_width;
    uint16_t source_height;
    uint16_t source_stride_pixels;

    uint16_t crop_x;
    uint16_t crop_y;
    uint16_t crop_width;
    uint16_t crop_height;

    uint16_t output_width;
    uint16_t output_height;

    uint8_t quality;
} image_jpeg_encode_cfg_t;

/*
 *[@name] ImageJpeg_AsyncInit
 *[@type] function
 *[@usage] 创建静态低优先级JPEG后台任务，由AI Thread在调度器启动后调用一次
 *[@argument] none
 *[@return] 成功返回FSP_SUCCESS，任务创建失败返回FSP_ERR_OUT_OF_MEMORY
 */
fsp_err_t ImageJpeg_AsyncInit(void);

/*
 *[@name] ImageJpeg_SubmitCapturedSnapshot
 *[@type] function
 *[@usage] 非阻塞提交当前RGB888快照，后台任务随后执行软件JPEG压缩
 *[@argument] quality JPEG质量，范围1到100
 *[@argument] frame_sequence JPEG对应的摄像头帧序号
 *[@return] 提交成功返回FSP_SUCCESS，任务忙或参数无效时返回对应FSP错误码
 */
fsp_err_t ImageJpeg_SubmitCapturedSnapshot(
    uint8_t quality,
    uint32_t frame_sequence);

/*
 *[@name] ImageJpeg_GetAsyncResult
 *[@type] function
 *[@usage] 非阻塞读取后台编码结果，并在取走结果后将编码状态恢复为IDLE
 *[@argument] p_frame_sequence 返回已完成JPEG对应的摄像头帧序号
 *[@argument] p_jpeg_size 返回有效JPEG长度，单位为字节
 *[@return] 返回后台编码结果，尚未完成时返回FSP_ERR_IN_USE
 */
fsp_err_t ImageJpeg_GetAsyncResult(
    uint32_t * p_frame_sequence,
    size_t * p_jpeg_size);

/*
 *[@name] ImageJpeg_CaptureRgb565Snapshot
 *[@type] function
 *[@usage] 将RGB565摄像头帧裁剪、缩放并转换到模块私有RGB888快照，返回后不再访问源帧
 *[@argument] p_source_rgb565 稳定RGB565帧首地址
 *[@argument] p_cfg 裁剪、缩放和输出尺寸配置
 *[@return] 成功返回FSP_SUCCESS，否则返回对应错误码
 */
fsp_err_t ImageJpeg_CaptureRgb565Snapshot(
    const uint16_t * p_source_rgb565,
    const image_jpeg_encode_cfg_t * p_cfg);

/*
 *[@name] ImageJpeg_EncodeCapturedSnapshot
 *[@type] function
 *[@usage] 将模块私有RGB888快照编码到模块私有JPEG缓冲区，不访问摄像头帧
 *[@argument] quality JPEG质量，范围1到100
 *[@argument] p_jpeg_size 返回有效JPEG字节数
 *[@return] 成功返回FSP_SUCCESS，否则返回对应错误码
 */
fsp_err_t ImageJpeg_EncodeCapturedSnapshot(
    uint8_t quality,
    size_t * p_jpeg_size);

/*
 *[@name] ImageJpeg_EncodeRgb565
 *[@type] function
 *[@usage] 将RGB565指定区域转换为RGB888，并阻塞编码到调用者提供的JPEG缓冲区
 *[@argument] p_source_rgb565 RGB565源帧缓冲区首地址
 *[@argument] p_cfg 裁剪、输出尺寸和JPEG质量配置
 *[@argument] p_rgb888_workspace RGB888中间工作缓冲区
 *[@argument] rgb888_workspace_size RGB888工作缓冲区容量，单位为字节
 *[@argument] p_jpeg_output JPEG输出缓冲区
 *[@argument] jpeg_output_capacity JPEG输出缓冲区容量，单位为字节
 *[@argument] p_jpeg_size 返回实际生成的JPEG字节数
 *[@return] 编码成功返回FSP_SUCCESS，否则返回对应错误码
 */
fsp_err_t ImageJpeg_EncodeRgb565(
    const uint16_t * p_source_rgb565,
    const image_jpeg_encode_cfg_t * p_cfg,
    uint8_t * p_rgb888_workspace,
    size_t rgb888_workspace_size,
    uint8_t * p_jpeg_output,
    size_t jpeg_output_capacity,
    size_t * p_jpeg_size);

/*
 *[@name] ImageJpeg_EncodeAndPublishRgb565
 *[@type] function
 *[@usage] 将RGB565图像阻塞编码到模块内部缓冲区并发布只读JPEG数据
 *[@argument] p_source_rgb565 RGB565源帧缓冲区首地址
 *[@argument] p_cfg 裁剪、缩放、输出尺寸和JPEG质量配置
 *[@argument] p_jpeg_size 返回实际生成并发布的JPEG字节数
 *[@return] 编码和格式检查成功返回FSP_SUCCESS，否则返回对应错误码
 */
fsp_err_t ImageJpeg_EncodeAndPublishRgb565(
    const uint16_t * p_source_rgb565,
    const image_jpeg_encode_cfg_t * p_cfg,
    size_t * p_jpeg_size);


/*
 *[@name] ImageJpeg_ConvertRgb565ToRgb888Scalar
 *[@type] function
 *[@usage] 使用标量C代码完成RGB565裁剪、最近邻缩放和RGB888转换，作为Helium版本的正确性参考
 *[@argument] p_source_rgb565 RGB565源帧缓冲区首地址
 *[@argument] p_cfg 源尺寸、行步长、裁剪区域和输出尺寸配置
 *[@argument] p_destination_rgb888 RGB888输出缓冲区首地址
 *[@argument] destination_size RGB888输出缓冲区容量，单位为字节
 *[@return] 转换成功返回FSP_SUCCESS，否则返回参数、尺寸或缓冲区错误码
 */
fsp_err_t ImageJpeg_ConvertRgb565ToRgb888Scalar(
    const uint16_t * p_source_rgb565,
    const image_jpeg_encode_cfg_t * p_cfg,
    uint8_t * p_destination_rgb888,
    size_t destination_size);

/*
 *[@name] ImageJpeg_ConvertRgb565ToRgb888Helium
 *[@type] function
 *[@usage] 使用Arm Helium MVE完成RGB565裁剪、最近邻缩放和RGB888转换
 *[@argument] p_source_rgb565 RGB565源帧缓冲区首地址
 *[@argument] p_cfg 源尺寸、行步长、裁剪区域和输出尺寸配置
 *[@argument] p_destination_rgb888 RGB888输出缓冲区首地址
 *[@argument] destination_size RGB888输出缓冲区容量，单位为字节
 *[@return] 转换成功返回FSP_SUCCESS，否则返回参数、尺寸或缓冲区错误码
 */
fsp_err_t ImageJpeg_ConvertRgb565ToRgb888Helium(
    const uint16_t * p_source_rgb565,
    const image_jpeg_encode_cfg_t * p_cfg,
    uint8_t * p_destination_rgb888,
    size_t destination_size);

/*
 *[@name] ImageJpeg_GetEncodedData
 *[@type] function
 *[@usage] 获取最近一次成功编码并发布的只读JPEG数据，数据在下一次编码开始前保持有效
 *[@argument] pp_jpeg_data 返回模块内部JPEG缓冲区的只读首地址
 *[@argument] p_jpeg_size 返回有效JPEG数据长度，单位为字节
 *[@return] 数据有效返回FSP_SUCCESS，否则返回参数或未初始化错误码
 */
fsp_err_t ImageJpeg_GetEncodedData(
    const uint8_t ** pp_jpeg_data,
    size_t * p_jpeg_size);

/* 将单通道 Gray8 图像编码为 baseline JPEG，供低码率实时图传使用。 */
fsp_err_t ImageJpeg_EncodeGray8(
    const uint8_t * p_gray8,
    uint16_t width,
    uint16_t height,
    uint8_t quality,
    uint8_t * p_jpeg_output,
    size_t jpeg_output_capacity,
    size_t * p_jpeg_size);

#endif /* IMAGE_UPLOAD_IMAGE_JPEG_ENCODER_H_ */
/*
 * ======================== 本项目使用的 Arm Helium MVE intrinsic 说明 ========================
 *
 * uint16x8_t
 *     128 位无符号整数向量类型，包含 8 个相互独立的 uint16_t 通道。
 *
 * mve_pred16_t
 *     16 位元素操作使用的谓词类型，记录 8 个 uint16_t 通道中哪些通道有效。
 *
 * vld1q_u16(address)
 *     从连续内存中读取 8 个 uint16_t，并装入一个 128 位向量寄存器。
 *
 * vctp16q(count)
 *     根据剩余元素数量生成 16 位通道谓词，用于处理不足 8 个像素的尾部数据。
 *
 * vldrhq_gather_shifted_offset_z_u16(base, offsets, predicate)
 *     以 base 为基地址，按 offsets 中的 uint16_t 元素索引聚集读取 RGB565 数据。
 *     shifted offset 会将索引自动乘以 2，谓词关闭的通道返回 0，避免尾部越界读取。
 *
 * vdupq_n_u16(value)
 *     将同一个 uint16_t 常数复制到 8 个向量通道，常用于生成位掩码。
 *
 * vshrq_n_u16(vector, bits)
 *     将 8 个 uint16_t 通道同时逻辑右移固定的 bits 位。
 *
 * vshlq_n_u16(vector, bits)
 *     将 8 个 uint16_t 通道同时左移固定的 bits 位。
 *
 * vandq_u16(a, b)
 *     对 a 和 b 的 8 个对应通道分别执行按位与，用于提取 RGB565 颜色字段。
 *
 * vorrq_u16(a, b)
 *     对 a 和 b 的 8 个对应通道分别执行按位或，用于完成颜色位复制扩展。
 *
 * vstrbq_scatter_offset_p_u16(base, offsets, values, predicate)
 *     将 values 各通道的低 8 位按照字节偏移 offsets 分散写入内存。
 *     只有谓词启用的通道会执行写入，用于生成 RGBRGB 交错排列并保护尾部边界。
 *
 * 详细定义以编译器附带的 arm_mve.h 和 Arm MVE Intrinsics Reference 为准。
 * ===========================================================================================
 */
