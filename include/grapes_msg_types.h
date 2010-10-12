#ifndef GRAPES_MSG_TYPES_H
#define GRAPES_MSG_TYPES_H


/**
 * @file msg_types.h
 * @brief Basic message type mappings.
 *
 * Here the definitions of the various SOM message types are collected.
 * WARNING: when the ML is used, message types are shared among all users
 * of the ML. Make sure there are no overlaps!
 *
 */
#define MSG_TYPE_TOPOLOGY   0x10
#define MSG_TYPE_CHUNK      0x11
#define MSG_TYPE_SIGNALLING 0x12
#define MSG_TYPE_TMAN       0x13

#endif	/* GRAPES_MSG_TYPES_H */
