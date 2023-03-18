
#ifndef SRC_BLE_H_
#define SRC_BLE_H_

#include <stdbool.h>
#include "stdio.h"
#include <sl_bgapi.h>

#define UINT8_TO_BITSTREAM(p, n) { *(p)++ = (uint8_t)(n); }

#define UINT32_TO_BITSTREAM(p, n) { *(p)++ = (uint8_t)(n); *(p)++ = (uint8_t)((n) >> 8); \
                                  *(p)++ = (uint8_t)((n) >> 16); *(p)++ = (uint8_t)((n) >> 24); }

#define UINT32_TO_FLOAT(m, e) (((uint32_t)(m) & 0x00FFFFFFU) | (uint32_t)((int32_t)(e) << 24))


// BLE Data Structure, save all of our private BT data in here.
// Modern C (circa 2021 does it this way)
// typedef ble_data_struct_t is referred to as an anonymous struct definition
typedef struct
{
    // values that are common to servers and clients
    bd_addr myAddress;
    uint8_t myAddressType;

    // values unique for server


    uint8_t advertisingSetHandle;
    uint8_t connectionHandle;
    bool connection_open; // true when in an open connection
    bool ok_to_send_htm_indications; // true when client enabled indications
    bool ok_to_send_button_indications; // true when client enabled indications for button
    bool indication_in_flight; // true when an indication is in-flight
    bool isBonded;    //true when devices are bonded


    uint32_t thermometer_service_handle;
    uint16_t thermometer_characteristic_handle;

    // values unique for client
} ble_data_struct_t;


// function prototypes
ble_data_struct_t* getBleDataPtr(void);


void updateGATTDB(uint32_t actual_temp);
void displayTemp();

void handle_ble_event(sl_bt_msg_t *evt);
//
#endif /* SRC_BLE_H_ */
