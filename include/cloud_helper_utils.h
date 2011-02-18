/*
 *  Copyright (c) 2010 Andrea Zito
 *
 *  This is free software; see lgpl-2.1.txt
 */

#ifndef CLOUD_HELPER_UTILS_H
#define CLOUD_HELPER_UTILS_H

#include "cloud_helper.h"
#include "net_helper.h"

#define DATA_SOURCE_NONE 0
#define DATA_SOURCE_NET 1
#define DATA_SOURCE_CLOUD 2

/**
 * @file cloud_helper_utils.h
 *
 * @brief Utilities function for managing cloud communications
 *
 * This files provides some utilities functions that simplifies
 * managing the addition of the cloud layer to the architecture
 *
 */

/**
 * @brief Wait for data from peers and cloud via thread
 *
 * This function handles the waiting for data by peers and cloud
 * in a threaded way, hiding the need to manually managing threads.
 *
 * @param[in] n A pointer to the nodeID for which waiting data
 * @param[in] cloud A pointer to the cloud_helper_context for which
 * waiting data
 * @param[in] tout The timeout for the waiting
 * @param[in] user_fds A pointer to an optional array of file
 * descriptior to monitor
 * @param[out] data_source A pointer which will be used to store the
 * source for the data
 * @return 1 if data was available, 0 otherwise
 */
int wait4any_threaded(struct nodeID *n, struct cloud_helper_context *cloud, struct timeval *tout, int *user_fds, int *data_source);


/**
 * @brief Wait for data from peers and cloud in a polling scheme
 *
 * This function handles the waiting for data by peers and cloud
 * by repeatedly waiting for a small time on each reasource.
 *
 * @param[in] n A pointer to the nodeID for which waiting data
 * @param[in] cloud A pointer to the cloud_helper_context for which
 * waiting data
 * @param[in] tout The timeout for the waiting
 * @param[in] step_tout The time to spend waiting in each step. If
 * NULL default is 100 milliseconds
 * @param[in] user_fds A pointer to an optional array of file
 * descriptior to monitor
 * @param[out] data_source A pointer which will be used to store the
 * source for the data
 * @return 1 if data was available, 0 otherwise
 */
int wait4any_polling(struct nodeID *n, struct cloud_helper_context *cloud, struct timeval *tout, struct timeval *step_tout, int *user_fds, int *data_source);

#endif
