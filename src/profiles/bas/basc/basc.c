/**
 ****************************************************************************************
 *
 * @file basc.c
 *
 * @brief Battery Service Client implementation.
 *
 * Copyright (C) RivieraWaves 2009-2012
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup BASC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_config.h"

#if (BLE_BATT_CLIENT)

#include "basc.h"
#include "basc_task.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct basc_env_tag **basc_envs;

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void basc_init(void)
{
	// Register BASC task into kernel
    task_basc_desc_register();

    // Reset all the profile role tasks
    PRF_CLIENT_RESET(basc_envs, BASC);
}

void basc_enable_cfm_send(struct basc_env_tag *basc_env, struct prf_con_info *con_info, uint8_t status)
{
    // Counter
    uint8_t svc_inst;

    // Send APP the details of the discovered attributes on BASC
    struct basc_enable_cfm * rsp = KE_MSG_ALLOC(BASC_ENABLE_CFM,
                                                con_info->appid, con_info->prf_id,
                                                basc_enable_cfm);

    rsp->conhdl = con_info->conhdl;
    rsp->status = status;

    if (status == PRF_ERR_OK)
    {
        rsp->bas_nb = basc_env->bas_nb;

        for (svc_inst = 0; svc_inst < basc_env->bas_nb; svc_inst++)
        {
            rsp->bas[svc_inst] = basc_env->bas[svc_inst];

            // Register BASC task in gatt for indication/notifications
            prf_register_atthdl2gatt(&basc_env->con_info, &basc_env->bas[svc_inst].svc);
        }

        // Go to connected state
        ke_state_set(con_info->prf_id, BASC_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(basc_envs, con_info->prf_id, BASC);
    }

    ke_msg_send(rsp);
}

void basc_error_ind_send(struct basc_env_tag *basc_env, uint8_t status)
{
    struct basc_error_ind *ind = KE_MSG_ALLOC(BASC_ERROR_IND,
                                              basc_env->con_info.appid, basc_env->con_info.prf_id,
                                              basc_error_ind);

    ind->conhdl    = basc_env->con_info.conhdl;
    //it will be an BASC status code
    ind->status    = status;

    // send the message
    ke_msg_send(ind);
}

#endif /* (BLE_BATT_CLIENT) */

/// @} BASC
