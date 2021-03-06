#pragma once

#define BTS_NET_PROTOCOL_VERSION 100

/** 
 * Define this to enable debugging code in the p2p network interface.
 * This is code that would never be executed in normal operation, but is
 * used for automated testing (creating artificial net splits, 
 * tracking where messages came from and when)
 */
#define ENABLE_P2P_DEBUGGING_API 1

/**
 * 512 kb
 */
#define MAX_MESSAGE_SIZE (524288)  
