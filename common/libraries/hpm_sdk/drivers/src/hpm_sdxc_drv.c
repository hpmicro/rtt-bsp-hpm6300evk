/*
 * Copyright (c) 2021-2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "hpm_sdxc_drv.h"


#define SDXC_TMCLK_IN_MHZ   (1UL)

#define SDXC_DMA_MAX_XFER_LEN_26BIT ((1UL << 26) - 4U)
#define SDXC_DMA_MAX_XFER_LEN_16BIT ((1UL << 16) - 4U)

#define SDXC_DMA_MAX_XFER_SIZE_IN_BYTES (512UL * 1024UL)

#define SDXC_SYS_DMA_ALIGN_LEN (4U)

enum {
    sdxc_cmd_only = (1UL << 0),
    sdxc_cmd_and_tx_data = (1UL << 1),
    sdxc_cmd_and_rx_data = (1UL << 2),
    sdxc_data_with_auto_cmd12 = (1UL << 3),
    sdxc_data_with_auto_cmd23 = (1UL << 4),
    sdxc_boot_data = (1UL << 5),
    sdxc_boot_data_continuous = (1UL << 6),
};


static void sdxc_set_transfer_config(SDXC_Type *base,
                                     uint32_t xfer_flags,
                                     uint32_t block_cnt,
                                     uint32_t *new_flags);


#if defined(SDXC_SOC_SUPPORT_PIO_MODE) && (SDXC_SOC_SUPPORT_PIO_MODE == 1)
static void sdxc_read_data_buf(SDXC_Type *base, uint32_t *data, uint32_t num_of_words);

static hpm_stat_t sdxc_read_via_data_buf_blocking(SDXC_Type *base, sdxc_data_t *data);

static void sdxc_write_data_buf(SDXC_Type *base, const uint32_t *data, uint32_t num_of_words);

static hpm_stat_t sdxc_write_via_data_buf_blocking(SDXC_Type *base, sdxc_data_t *data);
#endif

static hpm_stat_t sdxc_transfer_data_blocking(SDXC_Type *base, sdxc_data_t *data, bool enable_dma);

static hpm_stat_t sdxc_tuning_error_recovery(SDXC_Type *base);

static uint32_t sdxc_prepare_cmd_xfer(const sdxc_command_t *cmd);

static void sdxc_set_transfer_config(SDXC_Type *base,
                                           uint32_t xfer_flags,
                                           uint32_t block_cnt,
                                           uint32_t *new_flags)
{
    uint32_t flags = base->CMD_XFER & ~(SDXC_CMD_XFER_MULTI_BLK_SEL_MASK | SDXC_CMD_XFER_BLOCK_COUNT_ENABLE_MASK |
                                        SDXC_CMD_XFER_DATA_PRESENT_SEL_MASK | SDXC_CMD_XFER_DATA_XFER_DIR_MASK |
                                        SDXC_CMD_XFER_AUTO_CMD_ENABLE_MASK | SDXC_CMD_XFER_RESP_TYPE_SELECT_MASK |
                                        SDXC_CMD_XFER_DMA_ENABLE_MASK);

    if (IS_HPM_BITMASK_SET(xfer_flags, sdxc_boot_data_continuous)) {
        base->PROT_CTRL &= ~SDXC_PROT_CTRL_STOP_BG_REQ_MASK;
        base->PROT_CTRL |= SDXC_PROT_CTRL_CONTINUE_REQ_MASK;
    }

    if (IS_HPM_BITMASK_SET(xfer_flags, sdxc_cmd_and_rx_data)) {
        flags |= SDXC_CMD_XFER_DATA_XFER_DIR_MASK;
    }
    if (block_cnt > 1U) {
        flags |= SDXC_CMD_XFER_MULTI_BLK_SEL_MASK | SDXC_CMD_XFER_BLOCK_COUNT_ENABLE_MASK;
        if (IS_HPM_BITMASK_SET(xfer_flags, sdxc_data_with_auto_cmd12)) {
            flags |= SDXC_CMD_XFER_AUTO_CMD_ENABLE_SET(sdxc_auto_cmd12_enabled);
        }
    }

    if (IS_HPM_BITMASK_SET(xfer_flags, sdxc_data_with_auto_cmd23)) {
        flags |= SDXC_CMD_XFER_AUTO_CMD_ENABLE_SET(sdxc_auto_cmd23_enabled);
    }

    *new_flags = flags;
}


hpm_stat_t sdxc_receive_cmd_response(const SDXC_Type *base, sdxc_command_t *cmd)
{
    hpm_stat_t status = status_success;

    if (cmd->resp_type != sdxc_dev_resp_none) {

        cmd->response[0] = base->RESP[0];
        if (cmd->resp_type == sdxc_dev_resp_r2) {

            /* R3-R2-R1-R0 (lowest 8 bits are invalid bits) has the same format as R2 format in SD spec
             *  after removing internal CRC7 and end bit
             */
            uint32_t resp0 = base->RESP[0];
            uint32_t resp1 = base->RESP[1];
            uint32_t resp2 = base->RESP[2];
            uint32_t resp3 = base->RESP[3];
            cmd->response[0] = (resp0 << 8);
            cmd->response[1] = (resp1 << 8) | (resp0 >> 24);
            cmd->response[2] = (resp2 << 8) | (resp1 >> 24);
            cmd->response[3] = (resp3 << 8) | (resp2 >> 24);
        }

        if (SDXC_CMD_XFER_AUTO_CMD_ENABLE_GET(base->CMD_XFER) != 0) {
            cmd->auto_cmd_resp = base->RESP[3];
        }
    }

    /* check response flag */
    if ((cmd->resp_error_flags != 0U) &&
        ((cmd->resp_type == sdxc_dev_resp_r1) || (cmd->resp_type == sdxc_dev_resp_r1b) ||
         (cmd->resp_type == sdxc_dev_resp_r6) || (cmd->resp_type == sdxc_dev_resp_r5))) {
        if ((cmd->resp_error_flags & cmd->response[0]) != 0U) {
            status = status_sdxc_send_cmd_failed;
        }
    }

    return status;
}

#if defined(SDXC_SOC_SUPPORT_PIO_MODE) && (SDXC_SOC_SUPPORT_PIO_MODE == 1)
static void sdxc_read_data_buf(SDXC_Type *base, uint32_t *data, uint32_t num_of_words)
{
    if (sdxc_is_data_buf_readable(base)) {
        for (uint32_t i = 0; i < num_of_words; i++) {
            data[i] = sdxc_read_data(base);
        }
    }
}

static hpm_stat_t sdxc_read_via_data_buf_blocking(SDXC_Type *base, sdxc_data_t *data)
{
    uint32_t interrupt_status = 0;
    hpm_stat_t status = status_success;
    do {
        /* For multi-block transfer, the block size must be 4-byte aligned */
        if ((data->block_cnt > 1) && (data->block_size % sizeof(uint32_t) != 0)) {
            status = status_invalid_argument;
            break;
        }
        if (data->block_size % sizeof(uint32_t) != 0U) {
            data->block_size += sizeof(uint32_t) - (data->block_size % sizeof(uint32_t));
        }

        uint32_t words_per_block = data->block_size / sizeof(uint32_t);
        uint32_t remaining_blocks = data->block_cnt;
        uint32_t *read_buf = data->rx_data;
        while (remaining_blocks > 0) {
            uint32_t status_flags = SDXC_INT_STAT_BUF_RD_READY_MASK | SDXC_STS_DATA_ERR;
            /* Wait until data is ready or timeout event occurs */
            do {
                interrupt_status = sdxc_get_interrupt_status(base);
            } while (!IS_HPM_BITMASK_SET(interrupt_status, status_flags));

            if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_DATA_CRC_ERR_MASK)) {
                /* Handle Data CRC error */
                if (!data->enable_ignore_error) {
                    status = status_sdxc_data_crc_error;
                    break;
                }
                sdxc_clear_interrupt_status(base, SDXC_STS_DATA_ERR);
            } else if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_DATA_TOUT_ERR_MASK)) {
                /* Handle Data timeout error */
                status = status_sdxc_data_timeout_error;
                break;
            } else {
                /* Receive data block by block */
                sdxc_clear_interrupt_status(base, SDXC_INT_STAT_BUF_RD_READY_MASK);
                sdxc_read_data_buf(base, read_buf, words_per_block);
                read_buf += words_per_block;
                remaining_blocks--;
            }
        }

        sdxc_clear_interrupt_status(base, SDXC_INT_STAT_XFER_COMPLETE_MASK);

    } while (false);

    return status;
}

static void sdxc_write_data_buf(SDXC_Type *base, const uint32_t *data, uint32_t num_of_words)
{
    if (sdxc_is_data_buf_writable(base)) {
        for (uint32_t i = 0; i < num_of_words; i++) {
            sdxc_write_data(base, data[i]);
        }
    }
}

static hpm_stat_t sdxc_write_via_data_buf_blocking(SDXC_Type *base, sdxc_data_t *data)
{
    uint32_t interrupt_status = 0;
    hpm_stat_t status = status_success;
    do {
        /* For multi-block transfer, the block size must be 4-byte aligned */
        if ((data->block_cnt > 1) && (data->block_size % sizeof(uint32_t) != 0)) {
            status = status_invalid_argument;
            break;
        }
        if (data->block_size % sizeof(uint32_t) != 0U) {
            data->block_size += sizeof(uint32_t) - (data->block_size % sizeof(uint32_t));
        }

        uint32_t words_per_block = data->block_size / sizeof(uint32_t);
        uint32_t remaining_blocks = data->block_cnt;
        const uint32_t *write_buf = data->tx_data;
        while (remaining_blocks > 0) {
            uint32_t status_flags = SDXC_INT_STAT_BUF_WR_READY_MASK | SDXC_STS_DATA_ERR;
            /* Wait until write data is allowed or timeout event occurs */
            do {
                interrupt_status = sdxc_get_interrupt_status(base);
            } while (!IS_HPM_BITMASK_SET(interrupt_status, status_flags));

            if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_DATA_CRC_ERR_MASK)) {
                /* Handle Data CRC error */
                if (!data->enable_ignore_error) {
                    status = status_sdxc_data_crc_error;
                    break;
                }
                sdxc_clear_interrupt_status(base, SDXC_STS_DATA_ERR);
            } else if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_DATA_TOUT_ERR_MASK)) {
                /* Handle Data timeout error */
                status = status_sdxc_data_timeout_error;
                break;
            } else {
                /* Receive data block by block */
                sdxc_clear_interrupt_status(base, SDXC_INT_STAT_BUF_WR_READY_MASK);
                sdxc_write_data_buf(base, write_buf, words_per_block);
                write_buf += words_per_block;
                remaining_blocks--;
            }
        }

        sdxc_clear_interrupt_status(base, SDXC_INT_STAT_XFER_COMPLETE_MASK);

    } while (false);

    return status;
}
#endif

bool sdxc_is_bus_idle(const SDXC_Type *base)
{
    uint32_t busy_mask = SDXC_PSTATE_CMD_INHIBIT_MASK | SDXC_PSTATE_DAT_INHIBIT_MASK;

    return IS_HPM_BITMASK_CLR(base->PSTATE, busy_mask);
}

hpm_stat_t sdxc_get_capabilities(const SDXC_Type *base, sdxc_capabilities_t *capabilities)
{
    hpm_stat_t status = status_invalid_argument;
    do {
        HPM_BREAK_IF((base == NULL) || (capabilities == NULL));

        capabilities->capabilities1.U = base->CAPABILITIES1;
        capabilities->capabilities2.U = base->CAPABILITIES2;
        capabilities->curr_capabilities1.U = base->CURR_CAPABILITIES1;
        capabilities->curr_capabilities2.U = base->CURR_CAPABILITIES2;

        status = status_success;
    } while (false);

    return status;
}

static uint32_t sdxc_prepare_cmd_xfer(const sdxc_command_t *cmd)
{
    uint32_t cmd_xfer = SDXC_CMD_XFER_CMD_INDEX_SET(cmd->cmd_index);
    uint32_t flags = cmd->cmd_flags;

    if (cmd->cmd_type != sdxc_cmd_type_empty) {
        flags &= ~SDXC_CMD_XFER_RESP_TYPE_SELECT_MASK;
        uint32_t cmd_check_flags = SDXC_CMD_XFER_CMD_CRC_CHK_ENABLE_MASK | SDXC_CMD_XFER_CMD_IDX_CHK_ENABLE_MASK;
        if ((cmd->resp_type == sdxc_dev_resp_r1) || (cmd->resp_type == sdxc_dev_resp_r5) ||
            (cmd->resp_type == sdxc_dev_resp_r6) || (cmd->resp_type == sdxc_dev_resp_r7)) {
            flags |= SDXC_CMD_RESP_LEN_48 | cmd_check_flags;
        } else if ((cmd->resp_type == sdxc_dev_resp_r1b) || (cmd->resp_type == sdxc_dev_resp_r5b)) {
            flags |= SDXC_CMD_RESP_LEN_48B | cmd_check_flags;
        } else if (cmd->resp_type == sdxc_dev_resp_r2) {
            flags |= SDXC_CMD_RESP_LEN_136 | SDXC_CMD_XFER_CMD_CRC_CHK_ENABLE_MASK;
        } else if ((cmd->resp_type == sdxc_dev_resp_r3) || (cmd->resp_type == sdxc_dev_resp_r4)) {
            flags |= SDXC_CMD_RESP_LEN_48;
        } else {
            /* do nothing */
        }
        if (IS_HPM_BITMASK_SET(flags, SDXC_CMD_XFER_RESP_ERR_CHK_ENABLE_MASK)) {
            flags |= SDXC_CMD_XFER_RESP_INT_DISABLE_MASK;
        }
    }

    switch (cmd->cmd_type) {
    case sdxc_cmd_type_abort_cmd:
        flags |= SDXC_CMD_TYPE_ABORT;
        break;
    case sdxc_cmd_type_suspend_cmd:
        flags |= SDXC_CMD_TYPE_SUSPEND;
        break;
    case sdxc_cmd_tye_resume_cmd:
        flags |= SDXC_CMD_TYPE_RESUME;
        break;
    default:
        flags |= SDXC_CMD_TYPE_NORMAL;
        break;
    }

    cmd_xfer |= ((flags & (SDXC_CMD_XFER_CMD_TYPE_MASK | SDXC_CMD_XFER_CMD_CRC_CHK_ENABLE_MASK |
                           SDXC_CMD_XFER_CMD_IDX_CHK_ENABLE_MASK | SDXC_CMD_XFER_RESP_TYPE_SELECT_MASK |
                           SDXC_CMD_XFER_DATA_PRESENT_SEL_MASK | SDXC_CMD_XFER_DMA_ENABLE_MASK |
                           SDXC_CMD_XFER_DATA_XFER_DIR_MASK | SDXC_CMD_XFER_AUTO_CMD_ENABLE_MASK |
                           SDXC_CMD_XFER_MULTI_BLK_SEL_MASK | SDXC_CMD_XFER_BLOCK_COUNT_ENABLE_MASK |
                           SDXC_CMD_XFER_RESP_TYPE_MASK | SDXC_CMD_XFER_RESP_ERR_CHK_ENABLE_MASK)));
    return cmd_xfer;
}

hpm_stat_t sdxc_send_command(SDXC_Type *base, const sdxc_command_t *cmd)
{
    sdxc_clear_interrupt_status(base, ~0U);

    uint32_t cmd_xfer = sdxc_prepare_cmd_xfer(cmd);

    base->CMD_ARG = cmd->cmd_argument;
    base->CMD_XFER = cmd_xfer;

    return status_success;
}


hpm_stat_t sdxc_parse_interrupt_status(SDXC_Type *base)
{
    uint32_t interrupt_status = sdxc_get_interrupt_status(base);
    hpm_stat_t status = status_success;
    if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_STS_ERROR)) {
        if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_CMD_TOUT_ERR_MASK)) {
            status = status_sdxc_cmd_timeout_error;
        } else if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_CMD_CRC_ERR_MASK)) {
            status = status_sdxc_cmd_crc_error;
        } else if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_CMD_END_BIT_ERR_MASK)) {
            status = status_sdxc_cmd_end_bit_error;
        } else if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_CMD_IDX_ERR_MASK)) {
            status = status_sdxc_cmd_index_error;
        } else if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_DATA_TOUT_ERR_MASK)) {
            status = status_sdxc_data_timeout_error;
        } else if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_DATA_CRC_ERR_MASK)) {
            status = status_sdxc_data_crc_error;
        } else if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_DATA_END_BIT_ERR_MASK)) {
            status = status_sdxc_data_end_bit_error;
        } else if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_ADMA_ERR_MASK)) {
            status = status_sdxc_adma_error;
        } else if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_TUNING_ERR_MASK)) {
            status = status_sdxc_tuning_error;
        } else if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_RESP_ERR_MASK)) {
            status = status_sdxc_response_error;
        } else if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_BOOT_ACK_ERR_MASK)) {
            status = status_sdxc_boot_ack_error;
        } else if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_CARD_REMOVAL_MASK)) {
            status = status_sdxc_card_removed;
        } else if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_AUTO_CMD_ERR_MASK)) {
            uint32_t auto_cmd_err_mask = base->AC_HOST_CTRL & 0xFFFFUL;
            if (IS_HPM_BITMASK_SET(auto_cmd_err_mask, SDXC_AC_HOST_CTRL_AUTO_CMD12_NOT_EXEC_MASK)) {
                status = status_sdxc_autocmd_cmd12_not_exec;
            } else if (IS_HPM_BITMASK_SET(auto_cmd_err_mask, SDXC_AC_HOST_CTRL_AUTO_CMD_TOUT_ERR_MASK)) {
                status = status_sdxc_autocmd_cmd_timeout_error;
            } else if (IS_HPM_BITMASK_SET(auto_cmd_err_mask, SDXC_AC_HOST_CTRL_AUTO_CMD_CRC_ERR_MASK)) {
                status = status_sdxc_autocmd_cmd_crc_error;
            } else if (IS_HPM_BITMASK_SET(auto_cmd_err_mask, SDXC_AC_HOST_CTRL_AUTO_CMD_IDX_ERR_MASK)) {
                status = status_sdxc_autocmd_cmd_index_error;
            } else if (IS_HPM_BITMASK_SET(auto_cmd_err_mask, SDXC_AC_HOST_CTRL_AUTO_CMD_EBIT_ERR_MASK)) {
                status = status_sdxc_autocmd_end_bit_error;
            } else if (IS_HPM_BITMASK_SET(auto_cmd_err_mask, SDXC_AC_HOST_CTRL_AUTO_CMD_RESP_ERR_MASK)) {
                status = status_sdxc_autocmd_cmd_response_error;
            } else if (IS_HPM_BITMASK_SET(auto_cmd_err_mask, SDXC_AC_HOST_CTRL_CMD_NOT_ISSUED_AUTO_CMD12_MASK)) {
                status = status_sdxc_autocmd_cmd_not_issued_auto_cmd12;
            } else {
                status = status_sdxc_auto_cmd_error;
            }
        }  else {
            status = status_sdxc_error;
        }
    } else {
        status = status_success;
    }
    return status;
}

hpm_stat_t sdxc_wait_cmd_done(SDXC_Type *base, sdxc_command_t *cmd, bool polling_cmd_done)
{
    hpm_stat_t status = status_success;
    uint32_t interrupt_status = sdxc_get_interrupt_status(base);

    if (polling_cmd_done) {
        while (!IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_CMD_COMPLETE_MASK)) {
            interrupt_status = sdxc_get_interrupt_status(base);
            if ((interrupt_status & SDXC_STS_CMD_ERR) != 0) {
              status = sdxc_parse_interrupt_status(base);
              HPM_BREAK_IF(status != status_success);
            }
        }
        sdxc_clear_interrupt_status(base, SDXC_INT_STAT_CMD_COMPLETE_MASK);

        if (status == status_success) {
            status = sdxc_receive_cmd_response(base, cmd);
        }
    }

    return status;
}


static hpm_stat_t sdxc_transfer_data_blocking(SDXC_Type *base, sdxc_data_t *data, bool enable_dma)
{
    hpm_stat_t status = status_success;

#if !defined(SDXC_SOC_SUPPORT_PIO_MODE) || (SDXC_SOC_SUPPORT_PIO_MODE == 0)
    if (!enable_dma) {
        return status_invalid_argument;
    }
#endif

    if (enable_dma) {
        uint32_t interrupt_status = 0;
        uint32_t status_flags = SDXC_INT_STAT_XFER_COMPLETE_MASK | SDXC_STS_ERROR;
        while (!IS_HPM_BITMASK_SET(interrupt_status, status_flags)) {
            interrupt_status = sdxc_get_interrupt_status(base);
            if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_DMA_INTERRUPT_MASK)) {
                sdxc_clear_interrupt_status(base, SDXC_INT_STAT_DMA_INTERRUPT_MASK);
                if (SDXC_PROT_CTRL_DMA_SEL_GET(base->PROT_CTRL) == (uint32_t) sdxc_dmasel_sdma) {
                    base->ADMA_SYS_ADDR += data->block_size;
                }
            }
        }

        if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_TUNING_ERR_MASK)) {
            status = status_sdxc_transfer_data_failed;
        } else if (IS_HPM_BITMASK_SET(interrupt_status, SDXC_STS_DATA_ERR | SDXC_INT_STAT_ADMA_ERR_MASK)) {
            if ((!data->enable_ignore_error) ||
                IS_HPM_BITMASK_SET(interrupt_status, SDXC_INT_STAT_DATA_TOUT_ERR_MASK)) {
                status = status_sdxc_transfer_data_failed;
            }
        } else {
            /* Do nothing */
        }

        sdxc_clear_interrupt_status(base, SDXC_INT_STAT_DMA_INTERRUPT_MASK | SDXC_INT_STAT_TUNING_ERR_MASK);

    } else {
#if defined(SDXC_SOC_SUPPORT_PIO_MODE) && (SDXC_SOC_SUPPORT_PIO_MODE == 1)
        if (data->rx_data != NULL) {
            status = sdxc_read_via_data_buf_blocking(base, data);
            if (status != status_success) {
                return status;
            }
        } else {
            status = sdxc_write_via_data_buf_blocking(base, data);
            if (status != status_success) {
                return status;
            }
        }
#endif
    }

    return status;
}


void sdxc_init(SDXC_Type *base, const sdxc_config_t *config)
{
    sdxc_reset(base, sdxc_reset_all, 0x10000U);

    uint32_t prot_ctrl = base->PROT_CTRL;

    prot_ctrl &= ~(SDXC_PROT_CTRL_DMA_SEL_MASK | SDXC_PROT_CTRL_SD_BUS_VOL_VDD1_MASK);

    prot_ctrl |= SDXC_PROT_CTRL_SD_BUS_PWR_VDD1_MASK;

    sdxc_enable_tm_clock(base);
    sdxc_set_data_timeout(base, config->data_timeout, NULL);

    base->PROT_CTRL = prot_ctrl;

    /* Enable SD internal clock and the output clock */
    base->SYS_CTRL |= SDXC_SYS_CTRL_INTERNAL_CLK_EN_MASK;
    while (!IS_HPM_BITMASK_SET(base->SYS_CTRL, SDXC_SYS_CTRL_INTERNAL_CLK_STABLE_MASK)) {

    }
    base->SYS_CTRL |= SDXC_SYS_CTRL_PLL_ENABLE_MASK;
    while (!IS_HPM_BITMASK_SET(base->SYS_CTRL, SDXC_SYS_CTRL_INTERNAL_CLK_STABLE_MASK)) {

    }

    base->SYS_CTRL |= SDXC_SYS_CTRL_SD_CLK_EN_MASK;

    base->INT_STAT_EN = SDXC_STS_ALL_FLAGS;
    base->INT_SIGNAL_EN = 0UL;
    base->INT_STAT = SDXC_STS_ALL_FLAGS;

    /* Set Host to version 4, enable 26-bit ADMA2 length mode */
    base->AC_HOST_CTRL &= ~(SDXC_AC_HOST_CTRL_UHS_MODE_SEL_MASK | SDXC_AC_HOST_CTRL_SAMPLE_CLK_SEL_MASK);
    base->AC_HOST_CTRL |= SDXC_AC_HOST_CTRL_HOST_VER4_ENABLE_MASK | SDXC_AC_HOST_CTRL_ADMA2_LEN_MODE_MASK;
}

void sdxc_set_data_timeout(SDXC_Type *base, uint32_t timeout_in_ms, uint32_t *actual_timeout_ms)
{
    uint32_t field_value = 0xE;
    /*
     * Timeout calculation logic:
     * the base timeout tick is (1UL << 13),
     * assuming TMCLK is 24MHz, then the timeout_in_us = (1UL << 13) / 24
     */
    uint32_t timeout_in_us_unit = (1UL << 13U) / SDXC_TMCLK_IN_MHZ;
    uint32_t timeout_in_us = timeout_in_ms * 1000UL;
    for (uint32_t i = 0; i < 0xFU; i++) {
        uint32_t timeout_us_using_value_i = (timeout_in_us_unit << i);
        if (timeout_in_us < timeout_us_using_value_i) {
            field_value = i;
            break;
        }
    }
    sdxc_enable_interrupt_status(base, SDXC_INT_STAT_EN_DATA_TOUT_ERR_STAT_EN_MASK, false);
    base->SYS_CTRL = (base->SYS_CTRL & ~SDXC_SYS_CTRL_TOUT_CNT_MASK) | SDXC_SYS_CTRL_TOUT_CNT_SET(field_value);
    sdxc_enable_interrupt_status(base, SDXC_INT_STAT_EN_DATA_TOUT_ERR_STAT_EN_MASK, true);

    if (actual_timeout_ms != NULL) {
        *actual_timeout_ms = (timeout_in_us_unit << field_value) / 1000UL;
    }
}

void sdxc_set_mmc_boot_config(SDXC_Type *base, const sdxc_boot_config_t *config)
{
    uint32_t emmc_boot_reg = base->EMMC_BOOT_CTRL;

    emmc_boot_reg &= ~(SDXC_EMMC_BOOT_CTRL_BOOT_TOUT_CNT_MASK | SDXC_EMMC_BOOT_CTRL_MAN_BOOT_EN_MASK);

    if (config->enable_boot_ack) {
        emmc_boot_reg |= SDXC_EMMC_BOOT_CTRL_BOOT_ACK_ENABLE_MASK;
    }

    /* TODO: Auto stop at block gap, how to handle this? */

    uint32_t block_attr_reg = base->BLK_ATTR & ~(SDXC_BLK_ATTR_XFER_BLOCK_SIZE_MASK | SDXC_BLK_ATTR_BLOCK_CNT_MASK);

    block_attr_reg |=
            SDXC_BLK_ATTR_XFER_BLOCK_SIZE_SET(config->block_size) | SDXC_BLK_ATTR_BLOCK_CNT_SET(config->block_cnt);

    base->BLK_ATTR = block_attr_reg;
    base->EMMC_BOOT_CTRL = emmc_boot_reg;
}

void sdxc_set_data_config(SDXC_Type *base, sdxc_xfer_direction_t data_dir, uint32_t block_cnt, uint32_t block_size)
{
    uint32_t block_attr_reg = base->BLK_ATTR & ~(SDXC_BLK_ATTR_XFER_BLOCK_SIZE_MASK | SDXC_BLK_ATTR_BLOCK_CNT_MASK);

    block_attr_reg |= SDXC_BLK_ATTR_BLOCK_CNT_SET(block_cnt) | SDXC_BLK_ATTR_XFER_BLOCK_SIZE_SET(block_size);

    base->BLK_ATTR = block_attr_reg;

    if (data_dir == sdxc_xfer_dir_write) {
        base->CMD_XFER &= ~SDXC_CMD_XFER_DATA_XFER_DIR_MASK;
    } else {
        base->CMD_XFER |= SDXC_CMD_XFER_DATA_XFER_DIR_MASK;
    }
}

hpm_stat_t sdxc_set_dma_config(SDXC_Type *base, const sdxc_adma_config_t *dma_cfg, const uint32_t *data_addr)
{
    hpm_stat_t status = status_success;

    /* Set DMA mode */
    base->PROT_CTRL = (base->PROT_CTRL & ~SDXC_PROT_CTRL_DMA_SEL_MASK) | SDXC_PROT_CTRL_DMA_SEL_SET(dma_cfg->dma_type);

    if (dma_cfg->dma_type == sdxc_dmasel_sdma) {
        if (((uint32_t) data_addr % SDXC_SYS_DMA_ALIGN_LEN) != 0U) {
            status = status_sdxc_dma_addr_or_len_unaligned;
        } else {
            base->ADMA_SYS_ADDR = (uint32_t) data_addr;
        }
    } else if (dma_cfg->dma_type == sdxc_dmasel_adma2) {
        base->ADMA_SYS_ADDR = (uint32_t) dma_cfg->adma_desc_ptr;
    } else if (dma_cfg->dma_type == sdxc_dmasel_adma3) {
        base->ADMA_ID_ADDR = (uint32_t)dma_cfg->adma_desc_ptr;
    } else {
        status = status_invalid_argument;
    }

    return status;
}

hpm_stat_t sdxc_set_adma3_desc(sdxc_adma_config_t *dma_config, sdxc_adma3_xfer_list *adma3_xfer_list)
{
    hpm_stat_t status = status_invalid_argument;
    if ((dma_config == NULL) || (adma3_xfer_list == NULL)) {
        return status;
    }

    uint32_t num_cmds = 0;
    int32_t remaining_table_words = (int32_t) dma_config->adma_table_words;
    sdxc_adma3_xfer_list *current_list = adma3_xfer_list;
    while (current_list != NULL) {
        num_cmds++;
        current_list = current_list->next;
    }
    /* Place the integrated descriptors at the start of adma table */
    sdxc_adma3_integrated_desc_t *integrated_desc = (sdxc_adma3_integrated_desc_t *) dma_config->adma_table;
    dma_config->adma_desc_ptr = (uint32_t *) integrated_desc;
    uint32_t min_required_adma_table_words = num_cmds * SDXC_AMDA3_DESC_MIN_WORDS;
    if (dma_config->adma_table_words < min_required_adma_table_words) {
        return status_sdxc_adma_table_not_enough;
    }
    remaining_table_words -= (int32_t)(num_cmds * SDXC_ADMA3_INTEGRATED_DESC_WORDS);
    uint32_t *current_dma_desc = (uint32_t *) &integrated_desc[num_cmds];
    current_list = adma3_xfer_list;
    while (current_list != NULL) {
        /* Handle SDXC Xfer one by one */
        const sdxc_xfer_t *sdxc_xfer = current_list->sdxc_xfer;
        sdxc_command_t *cmd = sdxc_xfer->command;
        sdxc_data_t *xfer_data = sdxc_xfer->data;
        sdxc_adma3_cmd_sd_desc_t *cmd_desc = (sdxc_adma3_cmd_sd_desc_t *) current_dma_desc;
        current_dma_desc += SDXC_ADMA3_CMD_DESC_WORDS;
        if (remaining_table_words < 0) {
            status = status_sdxc_adma_table_not_enough;
            break;
        }
        sdxc_adma2_descriptor_t *adma2_desc = (sdxc_adma2_descriptor_t *) current_dma_desc;

        /* Prepare ADMA3 Command Descriptor  */
        /* Fill in block count */
        cmd_desc->entry[SDXC_ADMA3_CMD_DESC_IDX_32BIT_BLK_CNT].attr = SDXC_ADMA3_CMD_FOR_SD_DESC_ATTR;
        cmd_desc->entry[SDXC_ADMA3_CMD_DESC_IDX_32BIT_BLK_CNT].data = xfer_data->block_cnt;
        /* Fill in block size */
        cmd_desc->entry[SDXC_ADMA3_CMD_DESC_IDX_BLK_SIZE].attr = SDXC_ADMA3_CMD_FOR_SD_DESC_ATTR;
        cmd_desc->entry[SDXC_ADMA3_CMD_DESC_IDX_BLK_SIZE].data = xfer_data->block_size;
        /* Fill in command argument */
        cmd_desc->entry[SDXC_ADMA3_CMD_DESC_IDX_ARG].attr = SDXC_ADMA3_CMD_FOR_SD_DESC_ATTR;
        cmd_desc->entry[SDXC_ADMA3_CMD_DESC_IDX_ARG].data = cmd->cmd_argument;
        /* Fill in Command Xfer */
        cmd_desc->entry[SDXC_ADMA3_CMD_DESC_IDX_CMD_XFER].attr = SDXC_ADMA3_CMD_FOR_SD_DESC_ATTR;
        uint32_t cmd_xfer = sdxc_prepare_cmd_xfer(cmd) | SDXC_CMD_XFER_RESP_ERR_CHK_ENABLE_MASK |
                            SDXC_CMD_XFER_RESP_INT_DISABLE_MASK;
        if (cmd->resp_type == sdxc_dev_resp_r5) {
            cmd_xfer |= SDXC_CMD_XFER_RESP_TYPE_MASK;
        }
        if ((xfer_data->enable_auto_cmd12) || (xfer_data->enable_auto_cmd23)) {
            cmd_xfer |= SDXC_CMD_XFER_AUTO_CMD_ENABLE_SET(3);
        }
        cmd_desc->entry[SDXC_ADMA3_CMD_DESC_IDX_CMD_XFER].data = cmd_xfer;
        /* Prepare ADMA2 descriptor */
        sdxc_adma_config_t adma2_cfg;
        adma2_cfg.dma_type = sdxc_dmasel_adma2;
        adma2_cfg.adma_table = (uint32_t *) adma2_desc;
        adma2_cfg.adma_table_words = remaining_table_words;

        uint32_t adma_entries = 0;

        status = sdxc_set_adma2_desc(&adma2_cfg, xfer_data, &adma_entries);
        if (status != status_success) {
            break;
        }
        remaining_table_words -= (int32_t)(adma_entries * SDXC_ADMA2_DESC_WORDS);
        if (remaining_table_words < 0) {
            status = status_sdxc_adma_table_not_enough;
            break;
        }
        current_dma_desc += adma_entries * SDXC_ADMA2_DESC_WORDS;

        /* Prepare Integrated Descriptor */
        integrated_desc->cmd_desc_ptr = cmd_desc;
        integrated_desc->attr = SDXC_ADMA3_INTEGRATED_DESC_ATTR;
        if (current_list->next == NULL) {
            integrated_desc->attr |= SDXC_ADMA3_INTEGRATED_ATTR_END;
        } else {
            ++integrated_desc;
        }
        current_list = current_list->next;
    }

    return status;
}

hpm_stat_t sdxc_set_adma2_desc(sdxc_adma_config_t *dma_config, const sdxc_data_t *xfer_data, uint32_t *num_entries)
{
    hpm_stat_t status = status_success;
    do {
        uint32_t adma2_entries = 1;
        if ((dma_config == NULL) || (xfer_data == NULL)) {
            status = status_invalid_argument;
            break;
        }
        uint32_t remaining_adma_tbl_words = dma_config->adma_table_words;
        if (remaining_adma_tbl_words < SDXC_ADMA2_DESC_WORDS) {
            status = status_sdxc_adma_table_not_enough;
            break;
        }
        size_t data_bytes = xfer_data->block_cnt * xfer_data->block_size;
        if (data_bytes > SDXC_DMA_MAX_XFER_SIZE_IN_BYTES) {
            status = status_sdxc_xfer_size_exceeds_max_limit;
            break;
        }
        sdxc_adma2_descriptor_t *adma2_desc = (sdxc_adma2_descriptor_t *) dma_config->adma_table;
        const uint32_t *adma2_desc_ptr = (const uint32_t *) adma2_desc;
        dma_config->adma_desc_ptr = adma2_desc_ptr;
        /* Simple ADMA2 descriptor */
        if (!xfer_data->use_data_list) {
            uint8_t *data_buf = (uint8_t *) ((xfer_data->tx_data != NULL) ? xfer_data->tx_data : xfer_data->rx_data);
            if (!SDXC_IS_DMA_ALIGNED(data_buf) || !SDXC_IS_DMA_ALIGNED(data_bytes)) {
                status = status_sdxc_dma_addr_or_len_unaligned;
                break;
            }
            uint32_t xfer_data_size = data_bytes;
            /* Format each adma2 descriptor entry */
            adma2_desc->addr = (const uint32_t *) data_buf;
            adma2_desc->len_attr = 0U;
            adma2_desc->len_lower = xfer_data_size & 0xFFFFU;
            adma2_desc->len_upper = xfer_data_size >> 16;
            adma2_desc->len_attr |= SDXC_ADMA2_DESC_VALID_FLAG;
            adma2_desc->act = SDXC_ADMA2_DESC_TYPE_TRANS;
            adma2_desc->end = 1U; /* End of ADMA2 descriptor */
        } else {
            /* Complicated ADMA2 descriptor */
            uint32_t remaining_bytes = data_bytes;
            const sdxc_data_list_t *data_list = (xfer_data->tx_data_list != NULL)
                                                    ? xfer_data->tx_data_list
                                                    : xfer_data->rx_data_list;
            while (data_list != NULL) {
                if (remaining_adma_tbl_words < SDXC_ADMA2_DESC_WORDS) {
                    status = status_sdxc_adma_table_not_enough;
                    break;
                }
                if (!SDXC_IS_DMA_ALIGNED(data_list->data_addr) || !SDXC_IS_DMA_ALIGNED(data_list->data_size)) {
                    status = status_sdxc_dma_addr_or_len_unaligned;
                    break;
                }
                /* Prepare ADMA2 table by traversing the data list */
                if (remaining_bytes < data_list->data_size) {
                    status = status_invalid_argument; /* Invalid Data List */
                    break;
                }
                if (data_list->data_size > SDXC_DMA_MAX_XFER_SIZE_IN_BYTES) {
                    status = status_sdxc_xfer_size_exceeds_max_limit;
                    break;
                }
                uint8_t *data_buf = (uint8_t *) data_list->data_addr;
                uint32_t xfer_data_size = data_list->data_size;
                /* Format each adma2 descriptor entry */
                adma2_desc->addr = (const uint32_t *) data_buf;
                adma2_desc->len_attr = 0U;
                adma2_desc->len_lower = xfer_data_size & 0xFFFFU;
                adma2_desc->len_upper = xfer_data_size >> 16;
                adma2_desc->len_attr |= SDXC_ADMA2_DESC_VALID_FLAG;
                adma2_desc->act = SDXC_ADMA2_DESC_TYPE_TRANS;
                remaining_adma_tbl_words -= SDXC_ADMA2_DESC_WORDS;
                remaining_bytes -= data_list->data_size;
                if (data_list->next == NULL) {
                    if (remaining_bytes > 0) {
                        status = status_invalid_argument;
                    }
                    adma2_desc->end = 1U;
                    break;
                }
                data_list = data_list->next;
                ++adma2_desc;
                ++adma2_entries;
            }
        }
        if ((status == status_success) && (num_entries != NULL)) {
            *num_entries = adma2_entries;
        }
    } while (false);

    return status;
}

hpm_stat_t sdxc_set_adma_table_config(SDXC_Type *base,
                                      sdxc_adma_config_t *dma_cfg,
                                      sdxc_data_t *data_cfg,
                                      sdxc_command_t *cmd)
{
    hpm_stat_t status = status_fail;
    const uint32_t *data_buf;

    if (data_cfg->rx_data == NULL) {
        data_buf = (const uint32_t *) data_cfg->tx_data;
    } else {
        data_buf = (const uint32_t *) data_cfg->rx_data;
    }

    if (dma_cfg->dma_type == sdxc_dmasel_sdma) {
        status = status_success;
    } else if (dma_cfg->dma_type == sdxc_dmasel_adma2) {
        status = sdxc_set_adma2_desc(dma_cfg, data_cfg, NULL);
    } else if (dma_cfg->dma_type == sdxc_dmasel_adma3) {
        sdxc_adma3_xfer_list adma3_xfer_list;
        sdxc_xfer_t sdxc_xfer;
        sdxc_xfer.command = cmd;
        sdxc_xfer.data = data_cfg;
        adma3_xfer_list.next = NULL;
        adma3_xfer_list.sdxc_xfer = &sdxc_xfer;
        status = sdxc_set_adma3_desc(dma_cfg, &adma3_xfer_list);
    } else {
        status = status_invalid_argument;
    }

    if (status == status_success) {
        status = sdxc_set_dma_config(base, dma_cfg, data_buf);
    }

    return status;
}

bool sdxc_reset(SDXC_Type *base, sdxc_sw_reset_type_t reset_type, uint32_t timeout)
{
    uint32_t reset_mask = 0U;

    switch (reset_type) {
    case sdxc_reset_all:
        reset_mask = SDXC_SYS_CTRL_SW_RST_ALL_MASK;
        break;
    case sdxc_reset_cmd_line:
        reset_mask = SDXC_SYS_CTRL_SW_RST_CMD_MASK;
        break;
    case sdxc_reset_data_line:
        reset_mask = SDXC_SYS_CTRL_SW_RST_DAT_MASK;
        break;
    default:
        /* Do nothing */
        break;
    }
    base->SYS_CTRL |= reset_mask;
    while (IS_HPM_BITMASK_SET(base->SYS_CTRL, reset_mask)) {
        if (timeout == 0U) {
            return false;
        }
        timeout--;
    }

    return true;
}

void sdxc_select_voltage(SDXC_Type *base, sdxc_bus_voltage_option_t option)
{
    uint32_t option_u32 = (uint32_t) option;

    base->PROT_CTRL =
            (base->PROT_CTRL & ~SDXC_PROT_CTRL_SD_BUS_VOL_VDD1_MASK) | SDXC_PROT_CTRL_SD_BUS_VOL_VDD1_SET(option_u32);

    if (option == sdxc_bus_voltage_sd_1v8) {
        base->AC_HOST_CTRL |= SDXC_AC_HOST_CTRL_SIGNALING_EN_MASK;
    } else {
        base->AC_HOST_CTRL &= ~SDXC_AC_HOST_CTRL_SIGNALING_EN_MASK;
    }
}

void sdxc_enable_wakeup_event(SDXC_Type *base, sdxc_wakeup_event_t evt, bool enable)
{
    uint32_t evt_u32 = (uint32_t) evt;

    if (enable) {
        base->PROT_CTRL |= evt_u32;
    } else {
        base->PROT_CTRL &= ~evt_u32;
    }
}

void sdxc_set_data_bus_width(SDXC_Type *base, sdxc_bus_width_t width)
{
    uint32_t host_ctrl = base->PROT_CTRL & ~(SDXC_PROT_CTRL_EXT_DAT_XFER_MASK | SDXC_PROT_CTRL_DAT_XFER_WIDTH_MASK);

    if (width == sdxc_bus_width_1bit) {
        host_ctrl |= SDXC_PROT_CTRL_DAT_XFER_WIDTH_SET(0U);
    } else if (width == sdxc_bus_width_4bit) {
        host_ctrl |= SDXC_PROT_CTRL_DAT_XFER_WIDTH_SET(1U);
    } else if (width == sdxc_bus_width_8bit) {
        host_ctrl |= SDXC_PROT_CTRL_EXT_DAT_XFER_SET(1U);
    } else {
        /* Do nothing */
    }
    base->PROT_CTRL = host_ctrl;

    sdxc_enable_sd_clock(base, true);
}

uint32_t sdxc_get_data_bus_width(const SDXC_Type *base)
{
    uint32_t bus_width;
    uint32_t bus_width_mask = base->PROT_CTRL & (SDXC_PROT_CTRL_EXT_DAT_XFER_MASK | SDXC_PROT_CTRL_DAT_XFER_WIDTH_MASK);

    if (IS_HPM_BITMASK_SET(bus_width_mask, SDXC_PROT_CTRL_EXT_DAT_XFER_MASK)) {
        bus_width = 8;
    } else if (IS_HPM_BITMASK_SET(bus_width_mask, SDXC_PROT_CTRL_DAT_XFER_WIDTH_MASK)) {
        bus_width = 4;
    } else {
        bus_width = 1;
    }
    return bus_width;
}

void sdxc_set_speed_mode(SDXC_Type *base, sdxc_speed_mode_t mode)
{
    uint32_t mode_u32 = (uint32_t) mode;

    base->AC_HOST_CTRL =
            (base->AC_HOST_CTRL & ~SDXC_AC_HOST_CTRL_UHS_MODE_SEL_MASK) | SDXC_AC_HOST_CTRL_UHS_MODE_SEL_SET(mode_u32);
    if ((mode_u32 & 0xFU) > sdxc_sd_speed_sdr12) {
        base->PROT_CTRL |= SDXC_PROT_CTRL_HIGH_SPEED_EN_MASK;
    } else {
        base->PROT_CTRL &= ~SDXC_PROT_CTRL_HIGH_SPEED_EN_MASK;
    }
}

hpm_stat_t sdxc_transfer_nonblocking(SDXC_Type *base, sdxc_adma_config_t *dma_config, const sdxc_xfer_t *xfer)
{
    hpm_stat_t status = status_invalid_argument;
    sdxc_command_t *cmd = xfer->command;
    sdxc_data_t *data = xfer->data;

    uint32_t block_size = 0U;
    uint32_t block_cnt = 0U;
    do {
        uint32_t xfer_flags = (uint32_t) sdxc_cmd_only;
        base->PROT_CTRL &= ~SDXC_PROT_CTRL_DMA_SEL_MASK;
        cmd->cmd_flags &= ~(SDXC_CMD_XFER_DMA_ENABLE_MASK | SDXC_CMD_XFER_DATA_PRESENT_SEL_MASK | SDXC_CMD_XFER_DMA_ENABLE_MASK);

        if (data != NULL) {
#if !defined(SDXC_SOC_SUPPORT_PIO_MODE) || (SDXC_SOC_SUPPORT_PIO_MODE == 0)
            if ((dma_config == NULL) || (dma_config->dma_type == sdxc_dmasel_nodma)) {
                break;
            }
#endif
            block_size = data->block_size;
            block_cnt = data->block_cnt;
            xfer_flags = data->enable_auto_cmd12 ? (uint32_t) sdxc_data_with_auto_cmd12 : 0;
            xfer_flags |= (data->enable_auto_cmd23) ? (uint32_t) sdxc_data_with_auto_cmd23 : 0;
            if (!data->use_data_list) {
                xfer_flags |= (data->tx_data != NULL) ? (uint32_t) sdxc_cmd_and_tx_data : 0;
                xfer_flags |= (data->rx_data != NULL) ? (uint32_t) sdxc_cmd_and_rx_data : 0;
            } else {
                xfer_flags |= (data->tx_data_list != NULL) ? (uint32_t) sdxc_cmd_and_tx_data : 0;
                xfer_flags |= (data->rx_data_list != NULL) ? (uint32_t) sdxc_cmd_and_rx_data : 0;
            }
            cmd->cmd_flags |= SDXC_CMD_XFER_DATA_PRESENT_SEL_MASK;

            if (data->enable_auto_cmd23) {
                base->AC_HOST_CTRL |= SDXC_AC_HOST_CTRL_CMD23_ENABLE_MASK;
            } else {
                base->AC_HOST_CTRL &= ~SDXC_AC_HOST_CTRL_CMD23_ENABLE_MASK;
            }

            if ((dma_config != NULL) && (dma_config->dma_type != sdxc_dmasel_nodma)) {
                cmd->cmd_flags |= SDXC_CMD_XFER_DMA_ENABLE_MASK;
                uint32_t new_xfer_flags = 0;
                sdxc_set_transfer_config(base, xfer_flags, block_cnt, &new_xfer_flags);
                cmd->cmd_flags |= new_xfer_flags;

                status = sdxc_set_adma_table_config(base, dma_config, data, cmd);
                if (status != status_success) {
                    break;
                }
                if (dma_config->dma_type != sdxc_dmasel_adma3) {
                    base->BLK_ATTR = block_size;
                    base->SDMASA = block_cnt;
                }
            }
        }
        if ((dma_config == NULL) || (dma_config->dma_type != sdxc_dmasel_adma3)) {
            status = sdxc_send_command(base, cmd);
        }
    } while (false);

    return status;
}

hpm_stat_t sdxc_adma3_transfer_nonblocking(SDXC_Type *base,
                                           sdxc_adma_config_t *dma_config,
                                           sdxc_adma3_xfer_list *adma3_xfer_list)
{
    hpm_stat_t status = status_invalid_argument;
    do {
        if ((base == NULL) || (dma_config == NULL) || (dma_config->dma_type != sdxc_dmasel_adma3) || (
                adma3_xfer_list == NULL)) {
            break;
        }

        status = sdxc_set_adma3_desc(dma_config, adma3_xfer_list);
        if (status != status_success) {
            break;
        }

        status = sdxc_set_dma_config(base, dma_config, NULL);
    } while (false);
    return status;
}

hpm_stat_t sdxc_transfer_blocking(SDXC_Type *base, sdxc_adma_config_t *dma_config, const sdxc_xfer_t *xfer)
{
    hpm_stat_t status = status_invalid_argument;
    sdxc_command_t *cmd = xfer->command;
    sdxc_data_t *data = xfer->data;
    bool enable_dma = ((dma_config != NULL) && (dma_config->dma_type != sdxc_dmasel_nodma));
    do {
#if !defined(SDXC_SOC_SUPPORT_PIO_MODE) || (SDXC_SOC_SUPPORT_PIO_MODE == 0)
        if (!enable_dma) {
            break;
        }
#endif
        status = sdxc_transfer_nonblocking(base, dma_config, xfer);
        HPM_BREAK_IF(status != status_success);
        /* NOTE: adma3 transfer doesn't support the command done check */
        if ((dma_config == NULL) || (dma_config->dma_type != sdxc_dmasel_adma3)) {
            bool polling_cmd_done = (data == NULL);
            status = sdxc_wait_cmd_done(base, cmd, polling_cmd_done);
            if (status != status_success) {
                status = status_sdxc_send_cmd_failed;
                break;
            }
        }
        if (data != NULL) {
            status = sdxc_transfer_data_blocking(base, data, enable_dma);
        }
    } while (false);

    return status;
}

static hpm_stat_t sdxc_error_recovery_first_half(SDXC_Type *base)
{
    hpm_stat_t status = status_success;
    /* D8, D3-D0 of ERROR_INT_STAT */
    const uint32_t cmdline_err_mask = SDXC_INT_STAT_AUTO_CMD_ERR_MASK | SDXC_INT_STAT_CMD_TOUT_ERR_MASK
                                      | SDXC_INT_STAT_CMD_CRC_ERR_MASK | SDXC_INT_STAT_CMD_END_BIT_ERR_MASK;

    if ((base->INT_STAT & cmdline_err_mask) != 0U) {
        status = status_sdxc_send_cmd_failed;
        /* Only Auto Command error */
        if ((base->INT_STAT & cmdline_err_mask) == SDXC_INT_STAT_AUTO_CMD_ERR_MASK) {
            /* Will reset command line  */
        } else {
            if (((base->CMD_XFER & SDXC_CMD_XFER_RESP_INT_DISABLE_MASK) != 0U) ||
                ((base->AC_HOST_CTRL & SDXC_AC_HOST_CTRL_EXEC_TUNING_MASK))) {
                /* Will reset command line */
            } else {
                /* Wait until command completes */
                uint32_t timeout = 1000000UL;
                while (!IS_HPM_BITMASK_SET(base->INT_STAT, SDXC_INT_STAT_CMD_COMPLETE_MASK)) {
                    --timeout;
                    if (timeout < 1U) {
                        break;
                    }
                    if (IS_HPM_BITMASK_SET(base->INT_STAT, SDXC_INT_STAT_CMD_TOUT_ERR_MASK)) {
                        break;
                    }
                }
            }
        }

        sdxc_reset(base, sdxc_reset_cmd_line, 0xFFFFFFU);
    }

    /* Check D9, D6-D4 in ERR_INT_STAT */
    const uint32_t dataline_err_mask = SDXC_INT_STAT_ADMA_ERR_MASK | SDXC_INT_STAT_DATA_TOUT_ERR_MASK
                                       | SDXC_INT_STAT_DATA_CRC_ERR_MASK | SDXC_INT_STAT_DATA_END_BIT_ERR_MASK;

    if ((base->INT_STAT & dataline_err_mask) != 0U) {
        status = status_sdxc_transfer_data_failed;
        sdxc_reset(base, sdxc_reset_data_line, 0xFFFFFFU);
    }

    uint32_t error_status = sdxc_get_interrupt_status(base) & (cmdline_err_mask | dataline_err_mask);
    sdxc_clear_interrupt_status(base, error_status);

    if (error_status != 0U) {
        status = status_sdxc_error;
    }
    return status;
}

hpm_stat_t sdxc_error_recovery(SDXC_Type *base, sdxc_command_t *cmd)
{
    hpm_stat_t status = status_invalid_argument;
    do {
        if (base == NULL) {
            break;
        }

        status = sdxc_error_recovery_first_half(base);
        /* D8, D3-D0 of ERROR_INT_STAT */
        const uint32_t cmdline_err_mask = SDXC_INT_STAT_AUTO_CMD_ERR_MASK | SDXC_INT_STAT_CMD_TOUT_ERR_MASK
                                          | SDXC_INT_STAT_CMD_CRC_ERR_MASK | SDXC_INT_STAT_CMD_END_BIT_ERR_MASK;
        /* Check D9, D6-D4 in ERR_INT_STAT */
        const uint32_t dataline_err_mask = SDXC_INT_STAT_DATA_TOUT_ERR_MASK;

        if (cmd != NULL) {
            uint32_t cmd_index = cmd->cmd_index;
            if ((cmd_index == 12U) || (cmd_index == 52U)) {
                cmd->cmd_type = sdxc_cmd_type_abort_cmd;
            } else {
                cmd->cmd_type = sdxc_cmd_type_normal_cmd;
            }
            sdxc_send_command(base, cmd);
            /* Wait a while and then check the INHIBIT related bits */
            for (volatile uint32_t delay_cnt = 0; delay_cnt < 1000U; ++delay_cnt) {
                NOP();
            }
        }

        uint32_t timeout = 1000000UL;
        while (IS_HPM_BITMASK_SET(base->PSTATE, SDXC_PSTATE_DAT_INHIBIT_MASK | SDXC_PSTATE_CMD_INHIBIT_MASK)) {
            --timeout;
            if (timeout < 1U) {
                status = status_timeout;
                break;
            }
        }
        uint32_t int_stat = sdxc_get_interrupt_status(base);
        if ((int_stat & (cmdline_err_mask | dataline_err_mask)) != 0U) {
            status = status_sdxc_non_recoverable_error;
            break;
        }

        /* Wait more than 40us, assume each loop takes 20ns */
        timeout = 10000UL;
        while ((sdxc_get_interrupt_status(base) & dataline_err_mask) == 0) {
            timeout--;
            if (timeout < 1U) {
                break;
            }
        }
        uint32_t recoverable_data_line_mask = SDXC_PSTATE_DAT_3_0_MASK;
        if (sdxc_get_data_bus_width(base) == 8) {
            recoverable_data_line_mask |= SDXC_PSTATE_DAT_7_4_MASK;
        }
        if ((sdxc_get_present_status(base) & recoverable_data_line_mask) != recoverable_data_line_mask) {
            status = status_sdxc_non_recoverable_error;
            break;
        }
        if (status == status_sdxc_error) {
            status = status_sdxc_recoverable_error;
        }
    } while (false);

    return status;
}

hpm_stat_t sdxc_tuning_error_recovery(SDXC_Type *base)
{
    sdxc_reset_tuning_engine(base);
    sdxc_reset(base, sdxc_reset_data_line, 0xFFFFFFUL);
    sdxc_reset(base, sdxc_reset_cmd_line, 0xFFFFFFUL);

    return status_success;
}

hpm_stat_t sdxc_perform_tuning_flow_sequence(SDXC_Type *base, uint8_t tuning_cmd)
{
    hpm_stat_t status = status_success;

    /* Turn off Sampling clock */
    sdxc_enable_sd_clock(base, false);
    sdxc_execute_tuning(base);
    uint32_t block_size = SDXC_PROT_CTRL_EXT_DAT_XFER_GET(base->PROT_CTRL) ? 128U : 64U;
    sdxc_command_t cmd;
    (void) memset(&cmd, 0, sizeof(cmd));
    cmd.cmd_index = tuning_cmd;
    cmd.cmd_argument = 0;
    cmd.cmd_flags = SDXC_CMD_XFER_DATA_PRESENT_SEL_MASK | SDXC_CMD_XFER_DATA_XFER_DIR_MASK;
    cmd.resp_type = sdxc_dev_resp_r1;
    sdxc_enable_sd_clock(base, true);
    do {
        base->BLK_ATTR = block_size;
        base->SDMASA = 1;
        status = sdxc_send_command(base, &cmd);
        while (!IS_HPM_BITMASK_SET(base->INT_STAT, SDXC_INT_STAT_BUF_RD_READY_MASK)) {
        }
        sdxc_clear_interrupt_status(base, SDXC_INT_STAT_BUF_RD_READY_MASK);
    } while (IS_HPM_BITMASK_SET(base->AC_HOST_CTRL, SDXC_AC_HOST_CTRL_EXEC_TUNING_MASK));

    if (!IS_HPM_BITMASK_SET(base->AC_HOST_CTRL, SDXC_AC_HOST_CTRL_SAMPLE_CLK_SEL_MASK)) {
        sdxc_tuning_error_recovery(base);
        status = status_sdxc_tuning_failed;
    }

    return status;
}

hpm_stat_t sdxc_perform_software_tuning(SDXC_Type *base, uint8_t tuning_cmd)
{
    hpm_stat_t status;

    sdxc_tuning_error_recovery(base);

    /* Turn off Sampling clock */
    sdxc_enable_sd_clock(base, false);
    sdxc_reset_tuning_engine(base);
    uint32_t block_size = SDXC_PROT_CTRL_EXT_DAT_XFER_GET(base->PROT_CTRL) ? 128U : 64U;
    sdxc_command_t cmd;
    (void) memset(&cmd, 0, sizeof(cmd));
    cmd.cmd_index = tuning_cmd;
    cmd.cmd_argument = 0;
    cmd.cmd_flags = SDXC_CMD_XFER_DATA_PRESENT_SEL_MASK | SDXC_CMD_XFER_DATA_XFER_DIR_MASK;
    cmd.resp_type = sdxc_dev_resp_r1;
    base->BLK_ATTR = block_size;
    base->SDMASA = 0;
    uint32_t tuning_cclk_sel = 0;
    sdxc_enable_software_tuning(base, true);
    sdxc_set_center_phase_code(base, 0);
    sdxc_enable_sd_clock(base, true);

    bool center_phase_codes_valid[255];
    (void) memset(&center_phase_codes_valid, 0, sizeof(center_phase_codes_valid));

    do {

        sdxc_send_command(base, &cmd);

        uint32_t timeout_cnt = 0xFFFFFUL;
        while (!IS_HPM_BITMASK_SET(base->INT_STAT,
                                   SDXC_INT_STAT_BUF_RD_READY_MASK | SDXC_INT_STAT_ERR_INTERRUPT_MASK)) {
            timeout_cnt--;
            if (timeout_cnt < 1U) {
                break;
            }
        }

        if (IS_HPM_BITMASK_SET(base->INT_STAT, SDXC_INT_STAT_BUF_RD_READY_MASK)) {
            center_phase_codes_valid[tuning_cclk_sel] = true;
        }
        sdxc_clear_interrupt_status(base, ~0UL);

        sdxc_reset(base, sdxc_reset_cmd_line, 0xFFFFFFU);
        sdxc_reset(base, sdxc_reset_data_line, 0xFFFFFFU);

        tuning_cclk_sel++;

        sdxc_enable_sd_clock(base, false);
        sdxc_set_center_phase_code(base, tuning_cclk_sel);
        sdxc_enable_sd_clock(base, true);

    } while (tuning_cclk_sel < SDXC_AUTO_TUNING_STAT_CENTER_PH_CODE_GET(SDXC_AUTO_TUNING_STAT_CENTER_PH_CODE_MASK));

    int32_t first_window_idx = -1;
    int32_t last_window_idx = -1;
    for (int32_t i = 0; i < (int32_t) ARRAY_SIZE(center_phase_codes_valid); i++) {
        if (center_phase_codes_valid[i]) {
            first_window_idx = i;
            break;
        }
    }

    for (int32_t i = ARRAY_SIZE(center_phase_codes_valid) - 1; i >= 0; i--) {
        if (center_phase_codes_valid[i]) {
            last_window_idx = i;
            break;
        }
    }

    if ((first_window_idx >= 0) && (last_window_idx >= 0)) {

        uint32_t center_window = (first_window_idx + last_window_idx) / 2;
        sdxc_set_center_phase_code(base, center_window);

        status = status_success;
    } else {
        status = status_sdxc_tuning_failed;
    }

    return status;
}

hpm_stat_t sdxc_perform_auto_tuning(SDXC_Type *base, uint8_t tuning_cmd)
{
    bool need_inverse = sdxc_is_inverse_clock_enabled(base);
    sdxc_enable_inverse_clock(base, false);
    sdxc_enable_sd_clock(base, false);
    sdxc_enable_auto_tuning(base, true);
    sdxc_enable_inverse_clock(base, need_inverse);
    sdxc_enable_sd_clock(base, true);

    return sdxc_perform_tuning_flow_sequence(base, tuning_cmd);
}