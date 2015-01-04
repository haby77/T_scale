/**
 ****************************************************************************************
 *
 * @file app_com.h
 *
 * @brief UART transport module functions for Easy Application Controller Interface.
 *
 * Copyright (C) Quintic 2012-2013
 *
 * $Rev: 1.0 $
 *
 ****************************************************************************************
 */

#ifndef APP_PT_H_
#define APP_PT_H_

#include "app_env.h"

#define COM_FRAME_TIMEOUT 2 //2*10ms

#define QPPS_VAL_CHAR_NUM	(1)

#define EVENT_UART_TX_ID 3
#define EVENT_UART_RX_FRAME_ID 4
#define EVENT_UART_RX_TIMEOUT_ID 5

enum com_st
{
    COM_DEEPSLEEP	= 0,
    COM_ADV			= 1,
    COM_CONN_EMPTY   = 2,
    COM_CONN_FULL	= 3,
    
    COM_UART_TX_IDLE = 4,
    COM_UART_TX_ONGOING=5
};

struct com_env_tag
{
    uint8_t com_state ;
    
    ///Message id
    uint8_t msg_id;
    
    ///UART TX parameter 
    uint8_t tx_state;       //either transmitting or done.
    struct co_list queue_tx;///Queue of kernel messages corresponding to packets sent through HCI
    
    ///UART RX parameter 
    uint8_t com_rx_len;
    uint8_t com_rx_buf[QPPS_VAL_CHAR_NUM*QPP_DATA_MAX_LEN];

};

extern  struct com_env_tag  com_env;

extern uint32_t get_bit_num(uint32_t val);
extern void com_tx_done(void);
extern void com_event_uart_rx_frame_handler(void);
extern int app_com_uart_rx_done_ind_handler(ke_msg_id_t const msgid, void const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id);
extern int app_com_rx_timeout_handler(ke_msg_id_t const msgid, void const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id);
extern void com_event_uart_rx_timeout_handler(void);
extern void com_pdu_send(uint8_t len, uint8_t *par);
extern void com_init(void);

#endif
