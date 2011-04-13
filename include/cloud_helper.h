/*
 *  Copyright (c) 2010 Andrea Zito
 *
 *  This is free software; see lgpl-2.1.txt
 */

#ifndef CLOUD_HELPER_H
#define CLOUD_HELPER_H

#include <stdint.h>
#include <sys/time.h>

#include "net_helper.h"

/**
 * @file cloud_helper.h
 *
 * @brief Cloud communication facility interface.
 *
 * A clean interface throught which all the cloud communication procedure needed by SOM funcions
 * are handled.
 *
 * If thread are used, the caller must ensured that calls to cloud_helper_init
 * and get_cloud_helper_for are synchronized.
 */

/**
 * Implementation dependant internal representation of the cloud clontext
 */
struct cloud_helper_contex;

/**
 * @brief Initialize all needed internal parameters.
 * Initialize the parameters for the cloud facilities and create a context
 * representing the cloud. Only one instance of net_helper is allowed for a
 * specific nodeID.
 *
 * @param[in] local NodeID associated with this instance of cloud_helper.
 * @param[in] config Cloud specific configuration options.
 */
struct cloud_helper_context* cloud_helper_init(struct nodeID *local,
                                               const char *config);

/**
 * @brief Identifies the cloud_helper instance associated to the specified
 * nodeID
 * Returns the instance of cloud_helper that was initialized with the
 * specified nodeID.
 */
struct cloud_helper_context* get_cloud_helper_for(const struct nodeID *local);

/**
 * @brief Get the value for the specified key from the cloud.
 * This function send a request to the cloud for the value associated to the
 * specified key. Use the wait4cloud to listen for the answer and
 * revc_from_cloud to read the response.
 * @param[in] context The contex representing the desired cloud_helper
 *                    instance.
 * @param[in] key Key to retrieve.
 * @param[in] header_ptr A pointer to the header which will be added to the
 *                       retrieved data. May be NULL
 * @param[in] header_size The length of the header.
 * @param[in] free_header A positive value result in buffer_ptr being freed
 *                        on request completion
 * @return 0 if the request was successfully sent, 1 Otherwise
 */
int get_from_cloud(struct cloud_helper_context *context, const char *key,
                   uint8_t *header_ptr, int header_size, int free_header);

/**
 * @brief Get the value for key or return default value
 * This function send a request to the cloud for the value associated to the
 * specified key. Use the wait4cloud to listen for the answer and
 * revc_from_cloud to read the response.
 *
 * If the specified key isn't present on the cloud, return the default value
 * @param[in] context The contex representing the desired cloud_helper
 *                    instance.
 * @param[in] key Key to retrieve.
 * @param[in] header_ptr A pointer to the header which will be added to the
 *                       retrieved data. May be NULL
 * @param[in] header_size The length of the header.
 * @param[in] free_header A positive value result in buffer_ptr being freed
 *                        on request completion
 * @param[in] defval_ptr A pointer to the default value to use if key is missing
 * @param[in] defval_size The length of the default value
 * @param[in] free_defvar A positive value result in defval_ptr being freed on
                          request comletion
 * @return 0 if the request was successfully sent, 1 Otherwise
 */
int get_from_cloud_default(struct cloud_helper_context *context, const char *key,
                           uint8_t *header_ptr, int header_size, int free_header,
                           uint8_t *defval_ptr, int defval_size, int free_defval);

/**
 * @brief Returns the timestamp associated to the last GET operation.
 * Returns the timestamp associated to the last GET operation or NULL.
 * @param[in] context The contex representing the desired cloud_helper
 *                    instance.
 * @return timestamp ot NULL
 */
time_t timestamp_cloud(struct cloud_helper_context *context);

/**
 * @brief Put on the cloud the value for a specified key.
 * This function transparently handles the sending routines.
 * @param[in] context The contex representing the desired cloud_helper
 *                    instance.
 * @param[in] key Key to retrieve.
 * @param[in] buffer_ptr A pointer to the buffer in which to store the
 *                       retrieved data.
 * @param[in] buffer_size The size of the data buffer
 * @param[in] free_buffer A positive value result in buffer_ptr being freed
 *                        on request completion
 * @return 0 on success, 1 on failure
 */
int put_on_cloud(struct cloud_helper_context *context, const char *key,
                 uint8_t *buffer_ptr, int buffer_size, int free_buffer);

/**
 * @brief Returns the nodeID identifing the cloud for the specified variant.
 * This function transparently handles the identification of the cloud
 * node. Thanks to the variant parameter is possible to recover
 * nodeIDs which differ wrt to nodeid_equal, but are equal wrt to
 * is_cloud_node.
 * @param[in] context The contex representing the desired cloud_helper
 *                    instance.
 * @param[in] variant The variant number for this nodeID.
 * @return nodeID identifying the cloud.
 */
struct nodeID* get_cloud_node(struct cloud_helper_context *context,
                              uint8_t variant);

/**
 * @brief Check if the specified node references the cloud
 * This function transparently handles the comparation of cloud nodes.
 * @param[in] context The contex representing the desired cloud_helper
 *                    instance.
 * @param[in] node The supposed cloud node
 * @return 1 if cloud node, 0 otherwise
 */
int is_cloud_node(struct cloud_helper_context *context,
                  struct nodeID* node);

/**
 * @brief Check for cloud responses.
 * Check if the some cloud GET operation has concluded. It sets a timeout to
 * return at most after a given time.
 * @param[in] context The contex representing the desired cloud_helper
 *                    instance.
 * @param[in] tout A pointer to a timer to be used to set the waiting timeout.
 * @return 1 if the GET operation was succesful, -1 if the GET operation
 *         failed (unkwnown key), 0 otherwise.
 */
int wait4cloud(struct cloud_helper_context *context, struct timeval *tout);


/**
 * @brief Receive data from the cloud.
 * This function transparently handles the receving routines.
 * If the read bytes number is less than the buffer size or equal 0 than the
 * current response has been completely read.
 * @param[out] buffer_ptr A pointer to the buffer in which to store the retrieved data.
 * @param[out] buffer_size The size of the data buffer
 * @return The number of received bytes or -1 if some error occurred.
 */
int recv_from_cloud(struct cloud_helper_context *context, uint8_t *buffer_ptr, int buffer_size);

#endif
