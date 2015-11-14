/*
 * Copyright (C) 2015 INRIA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example application for demonstrating the basic
 *              IEEE 802.15.4 MAC layer for RIOT "gnrc" network stack
 *              Right now, this example only works for AT86RF2XX radios...
 *
 * @author      Kévin Roussel <Kevin.Roussel@inria.fr>
 *
 * @}
 */

#include <stdio.h>

#include "shell.h"
#include "board.h"
#include "msg.h"

#include "at86rf2xx.h"
#include "at86rf2xx_params.h"


#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

extern int basic_send_cmd(int argc, char **argv);
extern void start_frame_rx_server(void);


static at86rf2xx_t at86rf2xx_dev;
static kernel_pid_t basic_mac_tid;

static char[THREAD_STACKSIZE_DEFAULT] mac_thread_stack;

static const shell_command_t shell_commands[] = {
    { "send_frame", "send IEEE 802.15.4 mac frame", basic_send_cmd },
    { NULL, NULL, NULL }
};

int main(void)
{
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("RIOT IEEE 802.15.4 basic MAC layer example application.");

    /* I have to start all the network machinery here, since the
       'auto_init_' modules all sytematicaly start "nomac" */
    DEBUG("Initializing AT86RF2xx radio at SPI_0\n");
    const at86rf2xx_params_t *p = &at86rf2xx_params;
    int res = at86rf2xx_init(&at86rf2xx_dev,
                             p->spi,
                             p->spi_speed,
                             p->cs_pin,
                             p->int_pin,
                             p->sleep_pin,
                             p->reset_pin);
    if (res < 0) {
        core_panic(PANIC_ASSERT_FAIL,
                   "Error initializing AT86RF2xx radio device!");
    }
    else {
        /* start the 'gnrc_802154_basic_mac' module */
        basic_mac_tid = gnrc_802154_basic_mac_init(mac_thread_stack,
                                                   THREAD_STACKSIZE_DEFAULT,
                                                   THREAD_PRIORITY_MAIN - 4,
                                                   "802154_basic_mac",
                                                   (gnrc_netdev_t *)&at86rf2xx_dev);
    }

    /* Start IEEE 802.15.4 frame RX server */
    start_frame_rx_server();

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
