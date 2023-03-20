#include <stdbool.h>
#include "math.h"
#include "src/lcd.h"

#include "sl_bt_api.h"
#include "src/ble_device_type.h"
#include "src/ble.h"



#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"
#include "gatt_db.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "gpio.h"

#if DEVICE_IS_BLE_SERVER
#define MINIMUM_ADVERTISING_INTERVAL 400
#define MAXIMUM_ADVERTISING_INTERVAL 400

#define CONNECTION_INTERVAL_MINIMUM 60
#define CONNECTION_INTERVAL_MAXIMUM 60
#define SLAVE_LATENCY 4
#define SUPERVISION_TIMEOUT 150

#else
#define MINIMUM_ADVERTISING_INTERVAL 400
#define MAXIMUM_ADVERTISING_INTERVAL 400

#define CONNECTION_INTERVAL_MINIMUM 60
#define CONNECTION_INTERVAL_MAXIMUM 60
#define SLAVE_LATENCY 4
#define SUPERVISION_TIMEOUT 83
#define MIN_CE_LENGTH 0
#define MAX_CE_LENGTH 4

#define SCAN_INTERVAL                 80   //50ms
#define SCAN_WINDOW                   40   //25ms
#define SCAN_PASSIVE                  0


#endif

#define SOFT_TIMER_DURATION 32768
#define PBPRESS 8
#define PBRELEASE 16
uint32_t gattdbData;
uint32_t actual_temp_local;
uint8_t *char_value;

// BLE private data
ble_data_struct_t ble_data; // this is the declaration
uint8_t state[2];
int event;

// function that returns a pointer to the
// BLE private data
ble_data_struct_t* getBleDataPtr()
{

    return (&ble_data);


} // getBleDataPtr()


void handle_ble_event(sl_bt_msg_t *evt)
{
  ble_data.myAddressType=1;
  sl_status_t sc; // status code
  switch (SL_BT_MSG_ID(evt->header))
  {
    // ******************************************************
    // Events common to both Servers and Clients
    // ******************************************************
    // --------------------------------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack API commands before receiving this boot event!
    // Including starting BT stack soft timers!
    // --------------------------------------------------------

#if DEVICE_IS_BLE_SERVER
    case sl_bt_evt_system_boot_id:
      {

        state[0]=0x00;
        sl_bt_sm_delete_bondings();
        ble_data.indication_in_flight=0;
        ble_data.connection_open=0;
        ble_data.ok_to_send_htm_indications=0;
        ble_data.ok_to_send_button_indications=0;
        ble_data.isBonded=0;

        sc = sl_bt_system_get_identity_address(&ble_data.myAddress, &ble_data.myAddressType);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("sl_bt_system_get_identity_address() returned != 0 status=0x%04x", (unsigned int) sc);
          }

        // Create an advertising set.
        sc = sl_bt_advertiser_create_set(&ble_data.advertisingSetHandle);

        // Set advertising interval to 250ms.
        sc = sl_bt_advertiser_set_timing(ble_data.advertisingSetHandle,MINIMUM_ADVERTISING_INTERVAL,MAXIMUM_ADVERTISING_INTERVAL,0,0);

        // Start general advertising and enable connections.
        sc = sl_bt_advertiser_start(ble_data.advertisingSetHandle, sl_bt_advertiser_general_discoverable,
          sl_bt_advertiser_connectable_scannable);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x", (unsigned int) sc);
          }

        displayInit();                                //init the LCD display
        displayPrintf(DISPLAY_ROW_NAME, "Server",0);  //print server
        displayPrintf(DISPLAY_ROW_TEMPVALUE, "",0);   //clear temperature row
        displayPrintf(DISPLAY_ROW_BTADDR, "%0x:%0x:%0x:%0x:%0x:%0x ",
                       ble_data.myAddress.addr[0],
                       ble_data.myAddress.addr[1],
                       ble_data.myAddress.addr[2],
                       ble_data.myAddress.addr[3],
                       ble_data.myAddress.addr[4],
                       ble_data.myAddress.addr[5]);    //print server bt address
        displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising",0); //print current state
        displayPrintf(DISPLAY_ROW_ASSIGNMENT, "A7",0);    //print assignment number
        sl_bt_sm_configure(0b00001011, sm_io_capability_displayyesno);
        sl_status_t          timer_response;
        timer_response = sl_bt_system_set_soft_timer(SOFT_TIMER_DURATION,1,0);// set to 1s, handler 1 and recurring mode
        if (timer_response != SL_STATUS_OK) {
            LOG_ERROR("Timer soft error");
         }



      }// handle boot event

    break;






    case sl_bt_evt_connection_opened_id:
      {

        ble_data.connectionHandle=evt->data.evt_connection_opened.connection;

        sc = sl_bt_connection_set_parameters(ble_data.connectionHandle,CONNECTION_INTERVAL_MINIMUM,CONNECTION_INTERVAL_MAXIMUM,
                                        SLAVE_LATENCY, SUPERVISION_TIMEOUT,0,0xffff);
        if (sc != SL_STATUS_OK)
        {
          LOG_ERROR("sl_bt_connection_set_parameters() returned != 0 status=0x%04x", (unsigned int) sc);
        }
        sc= sl_bt_advertiser_stop(ble_data.advertisingSetHandle);
        if (sc != SL_STATUS_OK)
        {
          LOG_ERROR("sl_bt_advertiser_stop() returned != 0 status=0x%04x", (unsigned int) sc);
        }
        ble_data.connection_open=1;
        ble_data.indication_in_flight=0;

        displayPrintf(DISPLAY_ROW_CONNECTION, "Connected",0);//print current state
        //sl_bt_sm_configure(0b00001111, sm_io_capability_displayyesno);

        break;
      }// handle open event

    case sl_bt_evt_sm_confirm_bonding_id:
        sl_bt_sm_bonding_confirm(ble_data.connectionHandle,true);
        break;



    case sl_bt_evt_sm_confirm_passkey_id:
        {
        uint32_t key;
        key=evt->data.evt_sm_confirm_passkey.passkey;

        displayPrintf(DISPLAY_ROW_PASSKEY, "Passkey %d",key);//print current state
        displayPrintf(DISPLAY_ROW_ACTION, "Confirm with PB0",0);//print current state

        }
        break;

    case sl_bt_evt_sm_bonded_id:
        {
                displayPrintf(DISPLAY_ROW_PASSKEY, " ",0);//print current state
                displayPrintf(DISPLAY_ROW_ACTION, " ",0);//print current state
                displayPrintf(DISPLAY_ROW_CONNECTION, "Bonded",0);//print current state
                ble_data.isBonded=1;
        }
      break;

    case sl_bt_evt_system_external_signal_id:
      event=evt->data.evt_system_external_signal.extsignals;
      if(event==PBPRESS || event==PBRELEASE)
        {

      //LOG_INFO("inside external signal\n\r");



      //for bonding
      if(ble_data.isBonded==0)
        {


          if(event==PBPRESS)
            {
              displayPrintf(DISPLAY_ROW_9, "Button Pressed",0);//print current state
              sl_bt_sm_passkey_confirm(ble_data.connectionHandle,true);//confirm passkey
            }
          else if(event==PBRELEASE)
            {
              displayPrintf(DISPLAY_ROW_9, "Button Released",0);//print current state
            }
          break;

        }


      //for sending button state
      else
      {


          if(event==PBPRESS)
            {
              displayPrintf(DISPLAY_ROW_9, "Button Pressed",0);//print current state
              state[1]=0x01;
            }
          else if(event==PBRELEASE)
            {
              displayPrintf(DISPLAY_ROW_9, "Button Released",0);//print current state
              state[1]=0x00;
            }


          if(ble_data.connection_open==1 && ble_data.ok_to_send_button_indications==1 &&
             ble_data.isBonded==1 && ble_data.indication_in_flight==0)
             {
               sc = sl_bt_gatt_server_send_indication(ble_data.connectionHandle,
                                                      gattdb_button_state,
                                                      sizeof(state),
                                                      &state[0]);
               if (sc != SL_STATUS_OK)
                {
                   LOG_ERROR("sl_bt_advertiser_start release() returned != 0 status=0x%04x", (unsigned int) sc);
                }

               else
                 ble_data.indication_in_flight=1;
             }
           else if(ble_data.connection_open==1 && ble_data.ok_to_send_button_indications==1 &&
                   ble_data.isBonded==1 && ble_data.indication_in_flight==1)
             {
               //LOG_INFO("write queue in button\n\r");
               write_queue(gattdb_button_state,sizeof(state),state);
             }

          }



        }
      break;


    case sl_bt_evt_sm_bonding_failed_id:
      sl_bt_sm_bonding_confirm(ble_data.connectionHandle,true);
      break;

    case sl_bt_evt_connection_closed_id:
      {
        // Start general advertising and enable connections.
        sc = sl_bt_advertiser_start(ble_data.advertisingSetHandle, sl_bt_advertiser_general_discoverable,
                                    sl_bt_advertiser_connectable_scannable);

        if (sc != SL_STATUS_OK)
        {
          LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x", (unsigned int) sc);
        }
        ble_data.connection_open=0;
        ble_data.indication_in_flight=0;

        displayPrintf(DISPLAY_ROW_TEMPVALUE, "",0);
        displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising",0);//print current state
        sl_bt_sm_delete_bondings();


        break;

      }// handle close event







    case sl_bt_evt_connection_parameters_id:
      {
        struct sl_bt_evt_connection_parameters_s param_info;
        param_info.connection=(evt->data.evt_connection_parameters.connection);
        param_info.interval=(evt->data.evt_connection_parameters.interval);
        param_info.latency=(evt->data.evt_connection_parameters.latency);
        param_info.timeout=(evt->data.evt_connection_parameters.timeout);
        param_info.security_mode=(evt->data.evt_connection_parameters.security_mode);
//        LOG_INFO("/rconnection %d interval %d latency %d timeout %d security %d/r/n",param_info.connection,param_info.interval,param_info.latency,param_info.timeout,param_info.security_mode);
        (void) param_info;
      }


        break;


    case sl_bt_evt_gatt_server_characteristic_status_id:
      {
      if(evt->data.evt_gatt_server_characteristic_status.characteristic==gattdb_button_state)
        {
          if(evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config )
          {
            if(evt->data.evt_gatt_server_characteristic_status.client_config_flags==gatt_indication )
             {
                ble_data.ok_to_send_button_indications=1;
                gpioLed1SetOn();
             }
            else if(evt->data.evt_gatt_server_characteristic_status.client_config_flags==gatt_disable )
              {
                ble_data.ok_to_send_button_indications=0;
                gpioLed1SetOff();
              }
          }
        }

      if(evt->data.evt_gatt_server_characteristic_status.characteristic==gattdb_temperature_measurement)
        {
          if(evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config )
            {
              if(evt->data.evt_gatt_server_characteristic_status.client_config_flags==gatt_indication )
                {
                  ble_data.ok_to_send_htm_indications=1;
                  gpioLed0SetOn();
                }
              else if(evt->data.evt_gatt_server_characteristic_status.client_config_flags==gatt_disable )
                {
                  ble_data.ok_to_send_htm_indications=0;
                  gpioLed0SetOff();
                }

             }
         }

        if(evt->data.evt_gatt_server_characteristic_status.status_flags==sl_bt_gatt_server_confirmation)
          {
            ble_data.indication_in_flight=0;
          }

        if(ble_data.ok_to_send_htm_indications)
          {
            displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp=%d C",actual_temp_local);
          }
        else
          {
            displayPrintf(DISPLAY_ROW_TEMPVALUE, "",0);
          }

      }

          break;

    case sl_bt_evt_gatt_server_indication_timeout_id:

      ble_data.indication_in_flight=0;
      break;

#endif

////////////////////common/////////////

    case sl_bt_evt_system_soft_timer_id:
      //LOG_INFO("queue_depth %d\n\r",get_queue_depth());
      displayUpdate();
      uint16_t conHand;
      size_t conSize;
      uint8_t conBuff[5];
      if(get_queue_depth()!=0 && ble_data.indication_in_flight==0 &&
          (ble_data.ok_to_send_button_indications==1 || ble_data.ok_to_send_htm_indications))
        {

          LOG_INFO(" Inside dequeue %d\n\r",get_queue_depth());
          read_queue(&conHand,&conSize,&conBuff[0]);
          sc = sl_bt_gatt_server_send_indication(ble_data.connectionHandle,
                                                 conHand,
                                                 conSize,
                                                 &conBuff[0]);
          if (sc != SL_STATUS_OK)
            {
               LOG_ERROR("sl_bt_gatt_server_send_indication in timer() returned != 0 status=0x%04x", (unsigned int) sc);
            }
          else
            ble_data.indication_in_flight=1;
        }


      break;


////////////////////////////////////CLEINT/////////////////////////////////////////////////////////////
#if !DEVICE_IS_BLE_SERVER


          case sl_bt_evt_system_boot_id:
              sc = sl_bt_system_get_identity_address(&ble_data.myAddress, &ble_data.myAddressType);
              // Set passive scanning on 1Mb PHY
              sc = sl_bt_scanner_set_mode(sl_bt_gap_1m_phy, SCAN_PASSIVE);
              if (sc != SL_STATUS_OK)
              {
                LOG_ERROR("sl_bt_scanner_set_mode() returned != 0 status=0x%04x", (unsigned int) sc);
              }

              // Set scan interval and scan window
              sc = sl_bt_scanner_set_timing(sl_bt_gap_1m_phy, SCAN_INTERVAL, SCAN_WINDOW);
              if (sc != SL_STATUS_OK)
              {
                LOG_ERROR("sl_bt_scanner_set_timing() returned != 0 status=0x%04x", (unsigned int) sc);
              }

              // Set the default connection parameters for subsequent connections
              sc = sl_bt_connection_set_default_parameters(CONNECTION_INTERVAL_MINIMUM,
                                                           CONNECTION_INTERVAL_MAXIMUM,
                                                           SLAVE_LATENCY,
                                                           SUPERVISION_TIMEOUT,
                                                           MIN_CE_LENGTH,
                                                           MAX_CE_LENGTH);
              if (sc != SL_STATUS_OK)
              {
                LOG_ERROR("sl_bt_connection_set_default_parameters() returned != 0 status=0x%04x", (unsigned int) sc);
              }
              // Set scan interval and scan window
              sc = sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
              if (sc != SL_STATUS_OK)
              {
                LOG_ERROR("sl_bt_scanner_start() returned != 0 status=0x%04x", (unsigned int) sc);
              }


              displayInit();                                //init the LCD display
              displayPrintf(DISPLAY_ROW_NAME, "Client",0);  //print server
              displayPrintf(DISPLAY_ROW_TEMPVALUE, "",0);   //clear temperature row
              displayPrintf(DISPLAY_ROW_BTADDR, "%0x:%0x:%0x:%0x:%0x:%0x ",
                             ble_data.myAddress.addr[0],
                             ble_data.myAddress.addr[1],
                             ble_data.myAddress.addr[2],
                             ble_data.myAddress.addr[3],
                             ble_data.myAddress.addr[4],
                             ble_data.myAddress.addr[5]);    //print server bt address
              displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering",0); //print current state
              displayPrintf(DISPLAY_ROW_ASSIGNMENT, "A7",0);    //print assignment number
              break;
              sl_status_t          timer_response;
              timer_response = sl_bt_system_set_soft_timer(SOFT_TIMER_DURATION,1,0);// set to 1s, handler 1 and recurring mode
              if (timer_response != SL_STATUS_OK) {
                  LOG_ERROR("Timer soft error");
               }




          // This event is generated when an advertisement packet or a scan response
          // is received from a responder
          case sl_bt_evt_scanner_scan_report_id:
              if (evt->data.evt_scanner_scan_report.packet_type == 0 )
                {

                  //check if the server address is our device
                if(evt->data.evt_scanner_scan_report.address.addr[0]==SERVER_BT_ADDRESS.addr[0] &&
                   evt->data.evt_scanner_scan_report.address.addr[1]==SERVER_BT_ADDRESS.addr[1] &&
                   evt->data.evt_scanner_scan_report.address.addr[2]==SERVER_BT_ADDRESS.addr[2] &&
                   evt->data.evt_scanner_scan_report.address.addr[3]==SERVER_BT_ADDRESS.addr[3] &&
                   evt->data.evt_scanner_scan_report.address.addr[4]==SERVER_BT_ADDRESS.addr[4] &&
                   evt->data.evt_scanner_scan_report.address.addr[5]==SERVER_BT_ADDRESS.addr[5]    )
                    {
                      // then stop scanning for a while
                        sc = sl_bt_scanner_stop();
                        //set connection handle
                        ble_data.connectionHandle=1;
                      //connect to device
                        sc = sl_bt_connection_open(evt->data.evt_scanner_scan_report.address,
                                                   evt->data.evt_scanner_scan_report.address_type,
                                                   sl_bt_gap_1m_phy,
                                                   &ble_data.connectionHandle);
                    }

                }
              break;



          case sl_bt_evt_connection_opened_id:
            ble_data.connectionHandle=evt->data.evt_connection_opened.connection;
            displayPrintf(DISPLAY_ROW_BTADDR2, "%0x:%0x:%0x:%0x:%0x:%0x ",
                                  SERVER_BT_ADDRESS.addr[0],
                                  SERVER_BT_ADDRESS.addr[1],
                                  SERVER_BT_ADDRESS.addr[2],
                                  SERVER_BT_ADDRESS.addr[3],
                                  SERVER_BT_ADDRESS.addr[4],
                                  SERVER_BT_ADDRESS.addr[5]);
            displayPrintf(DISPLAY_ROW_CONNECTION, "Connected",0); //print current state
            break;







      // This event is generated when a new service is discovered
      case sl_bt_evt_gatt_service_id:

         // Save service handle for future reference
         ble_data.thermometer_service_handle = evt->data.evt_gatt_service.service;
         break;



     // This event is generated when a new characteristic is discovered
      case sl_bt_evt_gatt_characteristic_id:

         // Save characteristic handle for future reference
         ble_data.thermometer_characteristic_handle= evt->data.evt_gatt_characteristic.characteristic;

         break;






      case sl_bt_evt_gatt_characteristic_value_id:
        sc = sl_bt_gatt_send_characteristic_confirmation(evt->data.evt_gatt_characteristic_value.connection);
        displayPrintf(DISPLAY_ROW_CONNECTION, "Handling Indications",0); //print current state


        if (sc != SL_STATUS_OK)
        {
          LOG_ERROR("sl_bt_gatt_send_characteristic_confirmation() returned != 0 status=0x%04x", (unsigned int) sc);
        }

        char_value = &(evt->data.evt_gatt_characteristic_value.value.data[0]);
        displayTemp();
        break;


      case sl_bt_evt_connection_closed_id:
        sc = sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);

        if (sc != SL_STATUS_OK)
        {
          LOG_ERROR("sl_bt_scanner_start() returned != 0 status=0x%04x", (unsigned int) sc);
        }
        displayPrintf(DISPLAY_ROW_BTADDR2," ",0);
        displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering",0); //print current state
        displayPrintf(DISPLAY_ROW_TEMPVALUE, " ",0);
        break;

#endif

  } // end - switch

} // handle_ble_event()




void updateGATTDB(uint32_t actual_temp)
{





  actual_temp_local=actual_temp;
  uint8_t htm_temperature_buffer[5];
  uint8_t *p = htm_temperature_buffer;
  uint32_t htm_temperature_flt;
  uint8_t flags =0x00;


  UINT8_TO_BITSTREAM(p, flags);
  htm_temperature_flt = UINT32_TO_FLOAT(actual_temp*1000, -3);
  UINT32_TO_BITSTREAM(p, htm_temperature_flt);


  sl_status_t sc = sl_bt_gatt_server_write_attribute_value(
    gattdb_temperature_measurement, // handle from gatt_db.h
    0,                              // offset
    sizeof(htm_temperature_buffer), // length
    &htm_temperature_buffer[0]);    // pointer to buffer where data is


 // read_queue(&conHand,&conSize,&read_buffer[0]);

  if (sc != SL_STATUS_OK)
           LOG_ERROR("GATT DB WRITE ERROR");

  if(ble_data.connection_open==1 && ble_data.ok_to_send_htm_indications==1
      && ble_data.indication_in_flight==0)

    {


      sc = sl_bt_gatt_server_send_indication(ble_data.connectionHandle,
                                             gattdb_temperature_measurement,
                                             sizeof(htm_temperature_buffer),
                                             &htm_temperature_buffer[0]);

      if (sc != SL_STATUS_OK)
               LOG_ERROR("GATT INDICATION WRITE ERROR");
      else
        ble_data.indication_in_flight=1;

    }
  else if(ble_data.connection_open==1 && ble_data.ok_to_send_htm_indications==1 &&
          ble_data.indication_in_flight==1)
    {
      //LOG_INFO("TEMP queue write \n\r");
      write_queue(gattdb_temperature_measurement,sizeof(htm_temperature_buffer),htm_temperature_buffer);
    }


}

#if !DEVICE_IS_BLE_SERVER
// -----------------------------------------------
// Private function, original from Dan Walkes. I fixed a sign extension bug.
// We'll need this for Client A7 assignment to convert health thermometer
// indications back to an integer. Convert IEEE-11073 32-bit float to signed integer.
// -----------------------------------------------
static int32_t FLOAT_TO_INT32(const uint8_t *value_start_little_endian)
{
uint8_t signByte = 0;
int32_t mantissa;
// input data format is:
// [0] = flags byte
// [3][2][1] = mantissa (2's complement)
// [4] = exponent (2's complement)
// BT value_start_little_endian[0] has the flags byte
int8_t exponent = (int8_t)value_start_little_endian[4];
// sign extend the mantissa value if the mantissa is negative
if (value_start_little_endian[3] & 0x80) { // msb of [3] is the sign of the mantissa
signByte = 0xFF;
}
mantissa = (int32_t) (value_start_little_endian[1] << 0) |
(value_start_little_endian[2] << 8) |
(value_start_little_endian[3] << 16) |
(signByte << 24) ;
// value = 10^exponent * mantissa, pow() returns a double type
return (int32_t) (pow(10, exponent) * mantissa);
} // FLOAT_TO_INT32


void displayTemp()
{
  int32_t temperature;
  temperature=FLOAT_TO_INT32(char_value);
  displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp=%ld C",temperature);

}
#endif
















bool bufferFull=0;                  //flag for Full buffer
bool bufferEmpty=1;                 //flag for empty buffer
bool writeOperation=0;              //flag to denote write operation requested
bool readOperation=0;               //flag to denote read operation requested
bool firstOperation=1;              //flag to denote first read/write operation

static uint32_t bufferLength;
// Global variable for this assignment
// Declare memory for the queue/buffer/fifo, and the write and read pointers
queue_struct_t   my_queue[QUEUE_DEPTH]; // the queue - an array of structs
uint32_t         wptr = 0;              // write pointer
uint32_t         rptr = 0;              // read pointer

// Decide how will you handle the full condition.



// ---------------------------------------------------------------------
// Private function used only by this .c file.
// Compute the next ptr value. Given a valid ptr value, compute the next valid
// value of the ptr and return it.
// Isolation of functionality: This defines "how" a pointer advances.
// ---------------------------------------------------------------------
static uint32_t nextPtr(uint32_t ptr) {

  // Student edit:
  // Create this function
  if(firstOperation){           //if loop used to reset bufferEmpty flag on the first ever operation

      firstOperation=0;         //set flag to zero after completing first ever operation
      bufferEmpty=0;            //reset empty flag at first write operation
  }

  if(writeOperation){                   //if condition entered only if operation is write_queue()

    if((ptr+1)%QUEUE_DEPTH==rptr){      //if condition to check if buffer is full

        bufferFull=1;                   //bufferFull flag set only if buffer is full
        writeOperation=0;               //clear writeOperation flag as next wptr value computed
        return ((ptr+1)%QUEUE_DEPTH);   //increment and return pointer

    }
    else{

      bufferFull=0;                     //bufferFull flag is not set if buffer is not empty
      writeOperation=0;                 //clear writeOperation flag as next wptr value computed
      return ((ptr+1)%QUEUE_DEPTH);     //increment and return pointer
    }
  }

  else if(readOperation){               //if condition entered only if operation is read_queue()


    if((ptr+1)%QUEUE_DEPTH==wptr){      //if condition to check if buffer is empty

        bufferEmpty=1;                  //bufferEmpty flag set as buffer is empty
        readOperation=0;                //clear readOperation flag as next rptr value computed
        return ((ptr+1)%QUEUE_DEPTH);   //increment and return pointer
    }
    else{

      bufferEmpty=0;                    //bufferEmpty flag clear as buffer is empty
      readOperation=0;                  //clear readOperation flag as next rptr value computed
      return ((ptr+1)%QUEUE_DEPTH);     //increment and return pointer
    }

  }

  return 0;
} // nextPtr()


// ---------------------------------------------------------------------
// Public function
// This function writes an entry to the queue if the the queue is not full.
// Input parameter "a" should be written to queue_struct_t element "a"
// Input parameter "b" should be written to queue_struct_t element "b"
// Returns bool false if successful or true if writing to a full fifo.
// i.e. false means no error, true means an error occurred.
// ---------------------------------------------------------------------
bool write_queue (uint16_t charHandler, size_t buffersLength,uint8_t buffer[]) {

  // Student edit:
  // Create this function
  // Decide how you want to handle the "full" condition.

  // Isolation of functionality:
  //     Create the logic for "when" a pointer advances

  if(!bufferFull)                       //if buffer is not full

  {
    my_queue[wptr].charHandler=(uint16_t)charHandler;        //write first part of buffer
    my_queue[wptr].buffersLength=(size_t)buffersLength;       //write second part of the buffer
    for(int i=0;i<(int)buffersLength;i++)
    {
      my_queue[wptr].buffer[i]=(uint8_t)buffer[i];
    }

    bufferEmpty=0;                      //since write operation, empty buffer flag is cleared
    writeOperation=1;                   //setting writeOperation flag as this is a write operation
    wptr=nextPtr(wptr);                 //compute the next value of wptr
    bufferLength++;                     //used for keeping track of buffer used length

    return false;                       //return false when successful write

  }

    return true;                        //return true when writing to full buffer

} // write_queue()


// ---------------------------------------------------------------------
// Public function
// This function reads an entry from the queue.
// Write the values of a and b from my_queue[rptr] to the memory addresses
// pointed at by *a and *b. In this implementation, we do it this way because
// standard C does not provide a mechanism for a C function to return multiple
// values, as is common in perl or python.
// Returns bool false if successful or true if reading from an empty fifo.
// i.e. false means no error, true means an error occurred.
// ---------------------------------------------------------------------
bool read_queue (uint16_t *charHandler, size_t *buffersLength,uint8_t *buffer) {

  // Student edit:
  // Create this function

  // Isolation of functionality:
  //     Create the logic for "when" a pointer advances

    if(!bufferEmpty)                        //if buffer is not empty

      {
        *charHandler=(uint16_t)my_queue[rptr].charHandler;       //read first part of buffer
        *buffersLength=(size_t)my_queue[rptr].buffersLength;      //read second part of buffer
          for(int i=0;i<(int)my_queue[rptr].buffersLength;i++)
          {
            buffer[i]=(uint8_t)my_queue[rptr].buffer[i];

          }

        bufferFull=0;                       //since this is read operation, clear bufferFull flag
        readOperation=1;                    //as this is read op, set readOperation flag
        rptr=nextPtr(rptr);                 //compute next value of the rptr pointer
        bufferLength--;                     //decrement buffer length

        return false;                       //return false to denote successful read

      }


    return true;                            //return true if buffer is empty

} // read_queue()



// ---------------------------------------------------------------------
// Public function
// This function returns the wptr, rptr, full and empty values, writing
// to memory using the pointer values passed in, same rationale as read_queue()
// ---------------------------------------------------------------------
void get_queue_status (uint32_t *_wptr, uint32_t *_rptr, bool *_full, bool *_empty) {

  // Student edit:
  // Create this function

    *_wptr=wptr;                //write pointer
    *_rptr=rptr;                //read pointer
    *_full=bufferFull;          //buffer full flag
    *_empty=bufferEmpty;        //buffer empty flag




} // get_queue_status()



// ---------------------------------------------------------------------
// Public function
// Function that computes the number of written entries currently in the queue. If there
// are 3 entries in the queue, it should return 3. If the queue is empty it should
// return 0. If the queue is full it should return either QUEUE_DEPTH if
// USE_ALL_ENTRIES==1 otherwise returns QUEUE_DEPTH-1.
// ---------------------------------------------------------------------
uint32_t get_queue_depth() {

  // Student edit:
  // Create this function


    return bufferLength;        //return length of the used queue elements


} // get_queue_depth()



















