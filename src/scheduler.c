#include "app.h"
#include "em_core.h"
#include "sl_power_manager.h"
#include "src/i2c.h"
#include "src/timers.h"
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"
#include "sl_bt_api.h"
#include "src/scheduler.h"
#include "src/ble.h"
#include "src/ble_device_type.h"
#include "gatt_db.h"


// DOS: If the caller needs these, shouldn't they be defined in the .h file ???? !!!!!
#define UF_EVENT        1
#define COMP1_EVENT        2
#define I2C_COMPLETE_EVENT 4

#if !DEVICE_IS_BLE_SERVER
static const uint8_t thermo_service[2] = { 0x09, 0x18 };
static const uint8_t thermo_char[2] = { 0x1c, 0x2a };
#endif
ble_data_struct_t *ble_data2;

enum myStates_t
{
  IDLE,
  WAIT_FOR_TIMER,
  WAIT_FOR_I2C_COMPLETE
//  WAIT_FOR_TIMER_2,
//  WAIT_FOR_READ_COMPLETE

};


enum myDiscoveryStates_t
{
  STANDBY,
  PRIMARY_SERVICE,
  DISCOVER_CHARACTERISTIC,
  SET_NOTIFICATION
};

uint16_t nextEvent=1;
uint8_t i=0;
bool readOp=1;
bool writeOp=0;
uint32_t temperature=0;

//#define IDLE_EVENT 8
//#define EVENTE 16

//uint16_t eventLog;


//static int eventCompleted=0;

//bool ufEvent=0;
//bool comp1Event=0;
//bool i2cTransferComplete=0;
//bool writeCompleted=0;
//bool readCompleted=0;



uint16_t eventLog=0;



/**************************************************************************//**
 * Function to set the event for read temperature every 3s
 *****************************************************************************/

void setUFEvent()
{
 CORE_DECLARE_IRQ_STATE;
 CORE_ENTER_CRITICAL();
 sl_bt_external_signal(UF_EVENT);

 CORE_EXIT_CRITICAL();


}


void setCOMP1Event()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  sl_bt_external_signal(COMP1_EVENT);

  CORE_EXIT_CRITICAL();
}


void setI2CCompleteEvent()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  sl_bt_external_signal(I2C_COMPLETE_EVENT);

  CORE_EXIT_CRITICAL();
}


//void setSchedulerEvent()
//{
//  if(event==IDLE)
//    {
//
//    }
//}
/**************************************************************************//**
 * Function to read the events log and return an active event through priority
 * and clear the returned event
 *****************************************************************************/
//eventEnum lastEvent1 =IDLE_EVENT;
//eventEnum lastEvent2 =IDLE_EVENT;

uint8_t getEvent()
{

  uint8_t eventToReturn; // DOS

  // #########################
  // This code is broken, see fixed code below
  // #########################

//  if((eventLog & UF_EVENT) && (lastEvent1!=COMP0_UF))
//    {
//      CORE_DECLARE_IRQ_STATE;
//      CORE_ENTER_CRITICAL();
//      eventLog &= ~(1 << (UF_EVENT-1));
//      CORE_EXIT_CRITICAL();
////      nextEvent=2;
//      lastEvent1=COMP0_UF;
//      return COMP0_UF;
//    }
//
//  else if((eventLog & COMP1_EVENT))// && (lastEvent2!=COMP1_UF))
//    {
//       CORE_DECLARE_IRQ_STATE;
//       CORE_ENTER_CRITICAL();
//       eventLog &= ~(1 << (COMP1_EVENT-1));
//       CORE_EXIT_CRITICAL();
//       lastEvent2=COMP1_UF;
//
////       nextEvent=3;
//
//       return COMP1_UF;
//     }
//
//  else if((eventLog & I2C_COMPLETE_EVENT))
//    {
//      CORE_DECLARE_IRQ_STATE;
//      CORE_ENTER_CRITICAL();
//      eventLog &= ~(1 << (I2C_COMPLETE_EVENT-1));
//      CORE_EXIT_CRITICAL();
//
//      eventCompleted++;
//      if(eventCompleted%2==0)
//        {
//          lastEvent1=I2C_TRANSFER_COMPLETE;
//          lastEvent2=I2C_TRANSFER_COMPLETE;
//        }
//      else
//        {
//          lastEvent2=I2C_TRANSFER_COMPLETE;
//        }
//
//
//
////      nextEvent=4;
//
//
//
//      return I2C_TRANSFER_COMPLETE;
//    }
//
//  return 0;

  if (eventLog & UF_EVENT) {
      eventToReturn  = UF_EVENT;  // event to return
      eventLog      ^= UF_EVENT;  // clear the event in our private data structure
  }
  else if (eventLog & COMP1_EVENT) {
      eventToReturn  = COMP1_EVENT;  // event to return
      eventLog      ^= COMP1_EVENT;  // clear the event in our private data structure
  }
  else if (eventLog & I2C_COMPLETE_EVENT) {
      eventToReturn  = I2C_COMPLETE_EVENT;  // event to return
      eventLog      ^= I2C_COMPLETE_EVENT;  // clear the event in our private data structure
  }

  return (eventToReturn);


} // getEvent()

#if DEVICE_IS_BLE_SERVER

void temperature_state_machine(sl_bt_msg_t *evt)
{
  ble_data2=getBleDataPtr();
  if((SL_BT_MSG_ID(evt->header))==sl_bt_evt_system_external_signal_id && ble_data2->connection_open==1 &&
      ble_data2->ok_to_send_htm_indications==1)
    {

      enum myStates_t currentState=IDLE;
      static enum myStates_t nextState = IDLE;

      uint32_t event=evt->data.evt_system_external_signal.extsignals;


      currentState = nextState;

  //    event        = getEvent();                 //get event from scheduler

      switch(currentState)
            {

            case IDLE:
      //DOS        if(event & 1)
              if(event == 1)
                {
//                  gpioPD10On(); // monitor Expansion header pin 7 with a logic analyzer
//                  enableI2CGPIO();

      //            LOG_INFO("To1 %d", event);
                  nextState=WAIT_FOR_TIMER;
                  writeOp=1;
                  timerWaitUs_irq(80000);

                 }


              break;


            case WAIT_FOR_TIMER:
      //DOS        if( (event & 2) && (writeOp==1))
              if( (event == 2) && (writeOp==1))
                {
      //            LOG_INFO("To2a %d", event);
                  nextState=WAIT_FOR_I2C_COMPLETE;
                  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
                  startMeasurement();



                }
      //DOS        else if((event & 2) && (readOp==1) )
              else if((event == 2) && (readOp==1) )
                {
      //            LOG_INFO("To2b %d", event);
                  nextState=WAIT_FOR_I2C_COMPLETE;
                  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
                  readMeasurement();


                }
              break;


            case WAIT_FOR_I2C_COMPLETE:

      //DOS        if( (event & 3) && (writeOp==1) )
              if( (event == 4) && (writeOp==1) )
                {
                  NVIC_DisableIRQ(I2C0_IRQn);
                  writeOp=0;
                  readOp=1;
      //            LOG_INFO("BackTo1 %d", event);
                  nextState=WAIT_FOR_TIMER;
                  sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
                  timerWaitUs_irq(10800);

                }
      //DOS        else if( (event & 3) && (readOp==1) )
              else if( (event == 4) && (readOp==1) )
                {
                  NVIC_DisableIRQ(I2C0_IRQn);
      //            LOG_INFO("To0 %d", event);
                  nextState=IDLE;
                  readOp=0;
                  sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
                  temperature=dispTemperature();

                  //disableI2CGPIO();
                  updateGATTDB(temperature);
//                  gpioPD10Off();

                }
              break;

            } // switch
    }//if

}

#else

void discovery_state_machine(sl_bt_msg_t *evt)
{
  sl_status_t sc; // status code
  ble_data2=getBleDataPtr();
  enum myDiscoveryStates_t currentState=PRIMARY_SERVICE;
  static enum myDiscoveryStates_t nextState = PRIMARY_SERVICE;



  currentState = nextState;

  switch(currentState)
  {

  case PRIMARY_SERVICE:
    if( (SL_BT_MSG_ID(evt->header)==sl_bt_evt_connection_opened_id) )
      {

      // Discover Health Thermometer service on the responder device
        sc = sl_bt_gatt_discover_primary_services_by_uuid(evt->data.evt_connection_opened.connection,
                                                          sizeof(thermo_service),
                                                          (const uint8_t*)thermo_service);
        if (sc != SL_STATUS_OK)
         {
           LOG_ERROR("sl_bt_gatt_discover_primary_services_by_uuid() returned != 0 status=0x%04x", (unsigned int) sc);
         }
        nextState=DISCOVER_CHARACTERISTIC;
      }
    break;


  case DISCOVER_CHARACTERISTIC:
    if(SL_BT_MSG_ID(evt->header)==sl_bt_evt_gatt_procedure_completed_id)
      {
        // Discover thermometer characteristic on the responder device
        sc = sl_bt_gatt_discover_characteristics_by_uuid(evt->data.evt_gatt_procedure_completed.connection,
                                                         ble_data2->thermometer_service_handle,
                                                         sizeof(thermo_char),
                                                         (const uint8_t*)thermo_char);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("sl_bt_gatt_discover_characteristics_by_uuid() returned != 0 status=0x%04x", (unsigned int) sc);
          }
        nextState=SET_NOTIFICATION;
      }
    else if(SL_BT_MSG_ID(evt->header)==sl_bt_evt_connection_closed_id)
      nextState=PRIMARY_SERVICE;
    break;




  case SET_NOTIFICATION:
    if(SL_BT_MSG_ID(evt->header)==sl_bt_evt_gatt_procedure_completed_id)
      {
        // enable indications
          sc = sl_bt_gatt_set_characteristic_notification(evt->data.evt_gatt_procedure_completed.connection,
                                              ble_data2->thermometer_characteristic_handle,
                                              sl_bt_gatt_indication);

          if (sc != SL_STATUS_OK)
           {
             LOG_ERROR("sl_bt_gatt_discover_primary_services_by_uuid() returned != 0 status=0x%04x", (unsigned int) sc);
           }
        nextState=STANDBY;
      }
    else if(SL_BT_MSG_ID(evt->header)==sl_bt_evt_connection_closed_id)
          nextState=PRIMARY_SERVICE;
    break;

  case STANDBY:
    if(SL_BT_MSG_ID(evt->header)==sl_bt_evt_connection_closed_id)
      {


        nextState=PRIMARY_SERVICE;
      }
    break;


  }
}
#endif



