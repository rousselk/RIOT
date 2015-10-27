/*
 * Copyright (C) 2015 INRIA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     net_csma_sender
 * @file
 * @brief       Implementation of the CSMA/CA helper
 *
 * @author      Kévin Roussel <Kevin.Roussel@inria.fr>
 * @}
 */

#include <errno.h>
#include <stdbool.h>

#include "kernel.h"
#include "xtimer.h"
#include "net/gnrc/csma_sender.h"
#include "net/gnrc.h"
#include "net/netopt.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif


/** @brief Current value for macMinBE parameter */
static uint8_t macMinBE = MAC_MIN_BE_DEFAULT;

/** @brief Current value for macMaxBE parameter */
static uint8_t macMaxBE = MAC_MAX_BE_DEFAULT;

/** @brief Current value for macMaxCSMABackoffs parameter */
static uint8_t macMaxCSMABackoffs = MAC_MAX_CSMA_BACKOFFS_DEFAULT;


/*--------------------- "INTERNAL" UTILITY FUNCTIONS ---------------------*/

/**
 * @brief choose an adequate random backoff period in microseconds,
 *        from the given Backoff Exponent
 *
 * @param[in] be        Backoff Exponent for the computation of period
 *
 * @return              An adequate random backoff exponent in microseconds
 */
static inline uint32_t choose_backoff_period(int be)
{
    if (be < macMinBE) {
        be = macMinBE;
    }
    if (be > macMaxBE) {
        be = macMaxBE;
    }
    uint32_t max_backoff = ((1 << be) - 1) * A_UNIT_BACKOFF_PERIOD_uS;

    uint32_t period = genrand_uint32() % max_backoff;
    if (period < A_UNIT_BACKOFF_PERIOD_uS) {
        period = A_UNIT_BACKOFF_PERIOD_uS;
    }

    return period;
}


/*------------------------- "EXPORTED" FUNCTIONS -------------------------*/

void set_csma_mac_min_be(uint8_t val)
{
    macMinBE = val;
}

void set_csma_mac_max_be(uint8_t val)
{
    macMaxBE = val;
}

void set_csma_mac_max_csma_backoffs(uint8_t val);
{
    macMaxCSMABackoffs = val;
}


int csma_ca_send(gnrc_netdev_t *dev, gnrc_pktsnip_t *pkt);
{
    netopt_enable_t hwfeat;

    /* Does the transceiver do automatic CSMA/CA when sending? */
    int res = dev->driver->get(dev,
                               NETOPT_CSMA,
                               (void *) &hwfeat,
                               sizeof(netopt_enable_t));
    bool ok = false;
    switch (res) {
    case -ENODEV:
        /* invalid device pointer given */
        return -ENODEV;
    case -ENOTSUP:
        /* device doesn't make auto-CSMA/CA */
        ok = false;
        break;
    case -EOVERFLOW:  /* (normally impossible...*/
    case -ECANCELED:
        /* internal driver error! */
        return -ECANCELED;
    default:
        ok = (hwfeat == NETOPT_ENABLE);
    }

    if (ok) {
        /* device does CSMA/CA all by itself: let it do its job */
        DEBUG("Network device does hardware CSMA/CA\n");
        return dev->driver->send_data(dev, pktsnip);
    }

    /* if we arrive here, then we must perform the CSMA/CA procedure
       ourselves by software */
    genrand_init(xtimer_now());
    DEBUG("Starting software CSMA/CA....\n");

    int nb = 0, be = macMinBE;

    while (nb <= macMaxCSMABackoffs) {
        /* delay for an adequate random backoff period */
        uint32_t bp = choose_backoff_period(be);
        xtimer_usleep(bp);

        /* perform a CCA */
        DEBUG("Checking radio medium availability...\n");
        res = dev->driver->get(dev,
                               NETOPT_IS_CHANNEL_CLR,
                               (void *) &hwfeat,
                               sizeof(netopt_enable_t));
        if (res < 0) {
            /* normally impossible: we got a big internal problem! */
            return -ECANCELED;
        }

        /* if medium is clear, send the packet and return */
        if (hwfeat == NETOPT_ENABLE) {
            DEBUG("Radio medium available: sending packet.\n");
            return dev->driver->send_data(dev, pktsnip);
        }

        /* increment CSMA counters */
        DEBUG("Radio medium busy.\n");
        be++;
        if (be > macMaxBE) {
            be = macMaxBE;
        }
        nb++;
    }

    /* if we arrive here, medium was never available for transmission */
    DEBUG("Software CSMA/CA failure: medium never available.\n");
    return -EBUSY;
}


int cca_send(gnrc_netdev_t *dev, gnrc_pktsnip_t *pkt)
{
    netopt_enable_t hwfeat;

    /* Does the transceiver do automatic CCA before sending? */
    int res = dev->driver->get(dev,
                               NETOPT_AUTOCCA,
                               (void *) &hwfeat,
                               sizeof(netopt_enable_t));
    bool ok = false;
    switch (res) {
    case -ENODEV:
        /* invalid device pointer given */
        return -ENODEV;
    case -ENOTSUP:
        /* device doesn't make auto-CCA */
        ok = false;
        break;
    case -EOVERFLOW:  /* (normally impossible...*/
    case -ECANCELED:
        /* internal driver error! */
        return -ECANCELED;
    default:
        ok = (hwfeat == NETOPT_ENABLE);
    }

    if (ok) {
        /* device does auto-CCA: let him do its job */
        DEBUG("Network device does auto-CCA checking.\n");
        return dev->driver->send_data(dev, pktsnip);
    }

    /* if we arrive here, we must do CCA ourselves ro see if radio medium
       is clear before sending */
    DEBUG("Checking radio medium availability...\n");
    res = dev->driver->get(dev,
                           NETOPT_IS_CHANNEL_CLR,
                           (void *) &hwfeat,
                           sizeof(netopt_enable_t));
    if (res < 0) {
        /* normally impossible: we got a big internal problem! */
        return -ECANCELED;
    }

    /* if medium is clear, send the packet and return */
    if (hwfeat == NETOPT_ENABLE) {
        DEBUG("Radio medium available: sending packet.\n");
        return dev->driver->send_data(dev, pktsnip);
    }

    /* if we arrive here, medium was not available for transmission */
    DEBUG("Radio medium busy: transmission cancelled.\n");
    return -EBUSY;
}

