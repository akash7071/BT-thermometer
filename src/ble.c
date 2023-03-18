#include <stdbool.h>
#include "math.h"
#include "src/lcd.h"

#include "sl_bt_api.h"
#include "src/ble_device_type.h"
#include "src/ble.h"



#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"
#include "gatt_db.h"
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


#define PBPRESS 5
#define PBRELEASE 6
uint32_t gattdbData;
uint32_t actual_temp_local;
uint8_t *char_value;

// BLE private data
ble_data_struct_t ble_data; // this is the declaration


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

        sl_bt_sm_delete_bondings();
        ble_data.indication_in_flight=0;
        ble_data.connection_open=0;
        ble_data.ok_to_send_htm_indications=0;
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
        sl_bt_sm_configure(0b00001111, sm_io_capability_displayyesno);

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
      {
      int event=evt->data.evt_system_external_signal.extsignals;
      if(event==PBPRESS )
        {
          displayPrintf(DISPLAY_ROW_9, "Button Pressed",0);//print current state
          if(ble_data.isBonded==0)
            {
              sl_bt_sm_passkey_confirm(ble_data.connectionHandle,true);
            }
          else
            {
              uint8_t bond;
                        if(ble_data.isBonded==1)
                          bond=1;
                        else
                          bond=0;
              sc = sl_bt_gatt_server_send_indication(ble_data.connectionHandle,
                                                               gattdb_button_state,
                                                               sizeof(bond),
                                                               &bond);
                        if (sc != SL_STATUS_OK)
                                {
                                  LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x", (unsigned int) sc);
                                }
            }



        }
      if(event==PBRELEASE && ble_data.isBonded==1)
        {
          displayPrintf(DISPLAY_ROW_9, "Button Released",0);//print current state
          uint8_t bond;
          if(ble_data.isBonded==1)
            bond=1;
          else
            bond=0;

          uint8_t button_state_buffer[2];
          uint8_t *p = button_state_buffer;
          uint32_t button_state_flt;
          uint8_t flags =0x00;

          UINT8_TO_BITSTREAM(p, flags);
          button_state_flt = UINT32_TO_FLOAT(bond*1000, -3);
          UINT32_TO_BITSTREAM(p, button_state_flt);


          sl_status_t sc = sl_bt_gatt_server_write_attribute_value(
                                                          gattdb_button_state, // handle from gatt_db.h
                                                          0,                              // offset
                                                          sizeof(bond), // length
                                                          &bond);
          if (sc != SL_STATUS_OK)
                  {
                    LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x", (unsigned int) sc);
                  }

          sc = sl_bt_gatt_server_send_indication(ble_data.connectionHandle,
                                                 gattdb_button_state,
                                                 sizeof(bond),
                                                 &bond);
          if (sc != SL_STATUS_OK)
                  {
                    LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x", (unsigned int) sc);
                  }
        }
      }


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

      else if(evt->data.evt_gatt_server_characteristic_status.characteristic==gattdb_temperature_measurement)
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

          break;

    case sl_bt_evt_gatt_server_indication_timeout_id:

      ble_data.indication_in_flight=0;
      break;

#endif

////////////////////common/////////////

    case sl_bt_evt_system_soft_timer_id:
      displayUpdate();
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

  if (sc != SL_STATUS_OK)
           LOG_ERROR("GATT DB WRITE ERROR");

  if(ble_data.connection_open==1 && ble_data.ok_to_send_htm_indications==1
      && ble_data.indication_in_flight==0)
    {
      sc = sl_bt_gatt_server_send_indication(ble_data.connectionHandle,
                                             gattdb_temperature_measurement,
                                                   sizeof(htm_temperature_buffer),
                                                   &htm_temperature_buffer[0]);
      ble_data.indication_in_flight=1;
      if (sc != SL_STATUS_OK)
               LOG_ERROR("GATT INDICATION WRITE ERROR");

    }

}

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


