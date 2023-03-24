
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

    uint32_t buttonState_service_handle;
    uint16_t buttonState_characteristic_handle;

    // values unique for client
} ble_data_struct_t;


// function prototypes
ble_data_struct_t* getBleDataPtr(void);


void updateGATTDB(uint32_t actual_temp);
void displayTemp();

void handle_ble_event(sl_bt_msg_t *evt);
//












  // This is the number of entries in the queue. Please leave
  // this value set to 16.
  #define QUEUE_DEPTH      (16)
  // Student edit:
  //   define this to 1 if your design uses all array entries
  //   define this to 0 if your design leaves 1 array entry empty
  #define USE_ALL_ENTRIES  (1)




  // Modern C (circa 2021 does it this way)
  // This is referred to as an anonymous struct definition.
  // This is the structure of 1 queue/buffer/FIFO entry.
  // Please do not change this definition.
  typedef struct {
    uint16_t      charHandler;
    size_t        buffersLength;
    uint8_t       buffer[5];

  } queue_struct_t;





  // Function prototypes. The autograder (i.e. the testbench) only uses these
  // functions to test your design. Please do not change these definitions.
  bool     write_queue (uint16_t charHandler, size_t buffersLength,uint8_t buffer[]);
  bool     read_queue (uint16_t *charHandler, size_t *buffersLength,uint8_t *buffer);
  void     get_queue_status (uint32_t *_wptr, uint32_t *_rptr, bool *_full, bool *_empty);
  uint32_t get_queue_depth (void);



#endif /* SRC_BLE_H_ */
