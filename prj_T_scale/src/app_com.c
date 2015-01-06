/**
 ****************************************************************************************
 *
 * @file app_com.c
 *
 * @brief Pass through project process
 *
 * Copyright (C) Quintic 2012-2013
 *
 * $Rev: 1.0 $
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_env.h"
#include "app_com.h"
#include "uart.h"
#include "lib.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
 
 struct com_env_tag  com_env;
 extern uint32_t get_bit_num(uint32_t val);
 
  void com_init(void)
 {
    com_env.com_state = COM_DEEPSLEEP;

    //for com uart tx  
    com_env.tx_state = COM_UART_TX_IDLE;	//initialize tx state
    co_list_init(&com_env.queue_tx);			//init TX queue
     
    if(KE_EVENT_OK != ke_evt_callback_set(EVENT_UART_TX_ID, com_tx_done))
    ASSERT_ERR(0);
    if(KE_EVENT_OK != ke_evt_callback_set(EVENT_UART_RX_FRAME_ID, com_event_uart_rx_frame_handler))
    ASSERT_ERR(0);
    if(KE_EVENT_OK != ke_evt_callback_set(EVENT_UART_RX_TIMEOUT_ID, com_event_uart_rx_timeout_handler))
    ASSERT_ERR(0);

 }
 
 void app_event_com_tx_handler(void)
{
	ke_evt_set(1UL<<EVENT_UART_TX_ID);
}
 
 
 void com_uart_write(struct ke_msg *msg)
{
    //go to start tx state
    com_env.tx_state = COM_UART_TX_ONGOING;
		
    uart_write(QN_COM_UART, ((uint8_t *)&msg->param), msg->param_len, app_event_com_tx_handler);
		delay(0x1fff);
}

void com_event_uart_rx_timeout_handler(void)
{
	ke_timer_set(APP_COM_RX_TIMEOUT_TIMER, TASK_APP, COM_FRAME_TIMEOUT);

	ke_evt_clear(1UL << EVENT_UART_RX_TIMEOUT_ID);
}

int app_com_rx_timeout_handler(ke_msg_id_t const msgid, void const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    uart_rx_int_enable(QN_COM_UART, MASK_DISABLE);  //disable uart rx interrupt 
    struct app_uart_data_ind *com_data = ke_msg_alloc(APP_COM_UART_RX_DONE_IND,
                                 TASK_APP,
                                 TASK_APP,
                                 com_env.com_rx_len+1);
    com_data->len=com_env.com_rx_len;
    memcpy(com_data->data,com_env.com_rx_buf,com_env.com_rx_len);
    ke_msg_send(com_data);
    
    return (KE_MSG_CONSUMED);
}


void com_event_uart_rx_frame_handler(void)
{
	struct app_uart_data_ind *com_data = ke_msg_alloc(APP_COM_UART_RX_DONE_IND,
															 TASK_APP,
															 TASK_APP,
															 com_env.com_rx_len+1);
	com_data->len=com_env.com_rx_len;
	memcpy(com_data->data,com_env.com_rx_buf,com_env.com_rx_len);
	ke_msg_send(com_data);

	ke_timer_clear(APP_COM_RX_TIMEOUT_TIMER, TASK_APP);

	ke_evt_clear(1UL << EVENT_UART_RX_FRAME_ID);
}

int app_com_uart_rx_done_ind_handler(ke_msg_id_t const msgid, void const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    switch(msgid)
    {
        case APP_COM_UART_RX_DONE_IND:
        {
            struct app_uart_data_ind* frame = (struct app_uart_data_ind*)param;
            
            //for (uint8_t i=0;i<frame->len;i++)
            //QPRINTF("Cfm %d.\r\n", *(frame->data+i));
            
            ///leo test
//              pt_pdu_send(frame->len, &(frame->data[0]));
						///leo test end
            
            if(frame->len) //have data
            {    
                //calculate page num;
                 uint8_t pagket_res = frame->len%QPP_DATA_MAX_LEN;
                 uint8_t pagket_num;
                 if(pagket_res)
                 pagket_num = frame->len/QPP_DATA_MAX_LEN + 1;
                 else
                 pagket_num = frame->len/QPP_DATA_MAX_LEN;
                 
                 uint8_t cnt=0,sent_pagket=0; 

                for (cnt = 0; (sent_pagket<pagket_num) && cnt < QPPS_VAL_CHAR_NUM; cnt++)
                {
                     if ((app_qpps_env->char_status >> cnt) & QPPS_VALUE_NTF_CFG)
                     {
												app_qpps_env->char_status &= ~(QPPS_VALUE_NTF_CFG << cnt);
											 
                         if((pagket_res)&&(pagket_num-sent_pagket==1))
                         app_qpps_data_send(app_qpps_env->conhdl, cnt, pagket_res, (frame->data+sent_pagket*20));
                         else
                         app_qpps_data_send(app_qpps_env->conhdl, cnt, QPP_DATA_MAX_LEN, (frame->data+sent_pagket*20)); 
                         
                         sent_pagket++;
                     }
                }
            }
        }break;
        default :break;
    }
       
    return (KE_MSG_CONSUMED);
}

 
 
 /**
 ****************************************************************************************
 * @brief After-process when one PDU has been sent.
 *
 ****************************************************************************************
 */
void com_tx_done(void)
{
    struct ke_msg * msg;
    // Clear the event
    ke_evt_clear(1<<EVENT_UART_TX_ID);
    // Go back to IDLE state
    com_env.tx_state = COM_UART_TX_IDLE;
    //release current message (which was just sent)
    msg = (struct ke_msg *)co_list_pop_front(&com_env.queue_tx);
    // Free the kernel message space
    ke_msg_free(msg);
    // Check if there is a new message pending for transmission
    if ((msg = (struct ke_msg *)co_list_pick(&com_env.queue_tx)) != NULL)
    {
        // Forward the message to the HCI UART for immediate transmission
        com_uart_write(msg);
    }
}

// Push msg into eaci tx queue
static void com_push(struct ke_msg *msg)
{
    // Push the message into the list of messages pending for transmission
    co_list_push_back(&com_env.queue_tx, &msg->hdr);

    // Check if there is no transmission ongoing
    if (com_env.tx_state == COM_UART_TX_IDLE)
        // Forward the message to the HCI UART for immediate transmission
        com_uart_write(msg);
}

/**
 ****************************************************************************************
 * @brief EACI send PDU
 *
 ****************************************************************************************
 */
void com_pdu_send(uint8_t len, uint8_t *par)
{
    // Allocate one msg for EACI tx
    uint8_t *msg_param = (uint8_t*)ke_msg_alloc(0, 0, 0, len);

    // Save the PDU in the MSG
    memcpy(msg_param, par, len);

     //extract the ke_msg pointer from the param passed and push it in HCI queue
    com_push(ke_param2msg(msg_param));
}

//end
