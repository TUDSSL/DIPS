#include "pc-comms.h"

#include "platform.h"

#include "pb_decode.h"
#include "pb_encode.h"
#include "debugger-interface.pb.h"

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "supply.h"
#include "voltage-sense.h"
#include "current-sense.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

const uint8_t MagicBytes[] = {0x18 , 0xAA};

osThreadId_t pcCommsTaskHandle;
const osThreadAttr_t pcCommsTaskAttributes = {
  .name = "PcCommsReceive",
  .stack_size = 128 * 10,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
QueueHandle_t xPcCommsQueue;

osThreadId_t dataStreamProcessHandle;
const osThreadAttr_t dataStreamTaskAttributes = {
  .name = "SupplyRawDataIVTransmit",
  .stack_size = 128 * 10,
  .priority = (osPriority_t) osPriorityNormal1,
};

osThreadId_t statusStreamProcessHandle;
const osThreadAttr_t statusStreamTaskAttributes = {
        .name = "SupplyStatusTransmit",
        .stack_size = 128 * 10,
        .priority = (osPriority_t) osPriorityAboveNormal,
};

extern void handleEmulatorCalibrationRequest(Emulation_CalibrationCommand command);
extern void handleSupplyDataRequest(Supply_DataStreamRequest command);
extern void handleSupplyModeRequest(Supply_ModeRequest command);
extern void handleSupplyValueRequest(Supply_SupplyValueChangeRequest command);
extern void handleSupplyProfileDataResponds(Supply_ProfileDataRespond command);
extern void handleSupplyInitialDataRequest();

extern DataStreamsConfig dataStreamConfig;

void supplyTransmitRawIV(void);
void supplyTransmitStatus(void);
void supplyRequestDataProfile(void);


uint8_t upstream_buffer[Debugger_Upstream_size + BufferMargin + HeaderSize];

/**
 * Initialize pc Comm config
 */
void configPcComms(void){
  pcCommsTaskHandle = osThreadNew(pcCommsTask, NULL, &pcCommsTaskAttributes);
  dataStreamProcessHandle = osThreadNew(processDataStreams, NULL, &dataStreamTaskAttributes);
  statusStreamProcessHandle = osThreadNew(processStatusStreams, NULL, &statusStreamTaskAttributes);
  vTaskSuspend(dataStreamProcessHandle);
}

/**
 * FreeRTOS process datastream (status + measurements) to PC
 * @param argument
 */
void processDataStreams(void* argument){
  for(;;){
    bool activeStreams = false;
    osDelay(100);

    if(dataStreamConfig.rawIV){
      supplyTransmitRawIV();
      activeStreams = true;
    }

    if(!activeStreams){
      vTaskSuspend(dataStreamProcessHandle);
    }
  }
}

/**
 * FreeRTOS function for streaming current mode to the GUI
 * @param argument
 */
void processStatusStreams(void* argument){
    for(;;){
        osDelay(1000);
        supplyTransmitStatus();
    }
}

/**
 * Resume or suspend datastream process
 * @param status
 */
void setDataStreams(bool status){
  if(status){
    vTaskResume(dataStreamProcessHandle);
  } else {
    vTaskSuspend(dataStreamProcessHandle);
  }
}

extern osThreadId_t replayTaskHandle;

/**
 * RTOS function for Downstream communcation from GUI
 * @param argument
 */
void pcCommsTask(void* argument){
  // create a queue for 10 packets
  xPcCommsQueue = xQueueCreate( 8,  Debugger_Downstream_size);
  Debugger_Downstream downstreamMsg;
  for(;;){
    while(xQueueReceive(xPcCommsQueue, &downstreamMsg, portMAX_DELAY ) == pdPASS){
      switch(downstreamMsg.which_packet){
        case Debugger_Downstream_calibration_request_tag:{
          setDataStreams(false);
          vTaskSuspend(replayTaskHandle);
          handleEmulatorCalibrationRequest(downstreamMsg.packet.calibration_request.request);
          vTaskResume(replayTaskHandle);
          setDataStreams(true);
          break;
        }
        case Debugger_Downstream_datastream_request_tag:{
          handleSupplyDataRequest(downstreamMsg.packet.datastream_request);
          break;
        }
        case Debugger_Downstream_mode_request_tag: {
          handleSupplyModeRequest(downstreamMsg.packet.mode_request);
          break;
        }
        case Debugger_Downstream_supply_value_request_tag: {
          handleSupplyValueRequest(downstreamMsg.packet.supply_value_request);
          break;
        }
        case Debugger_Downstream_pause_request_tag: {
            if (downstreamMsg.packet.pause_request.request == Supply_PauseCommand_Pause)
                pauseResumeSupply(true, false);
            else if (downstreamMsg.packet.pause_request.request == Supply_PauseCommand_Resume)
                pauseResumeSupply(false, false);
            break;
        }
        case Debugger_Downstream_profile_data_respond_tag: {
            handleSupplyProfileDataResponds(downstreamMsg.packet.profile_data_respond);
            break;
        }
        case Debugger_Downstream_initial_param_request_tag: {
          // Request initial values of parameters
          handleSupplyInitialDataRequest();
          break;
        }
      }
    }
  }
}

/**
 * Transmit upstream packet to GUI
 * @param upstreamMsg
 * @return status
 */
bool transmitUpstreamPacket(Debugger_Upstream upstreamMsg)
{
    upstream_buffer[0] = MagicBytes[0];
    upstream_buffer[1] = MagicBytes[1];

    pb_ostream_t stream =
            pb_ostream_from_buffer(&upstream_buffer[2], Debugger_Upstream_size + BufferMargin);

    bool status = pb_encode_delimited(&stream, Debugger_Upstream_fields,
                                      &upstreamMsg);

    if(status){
        CDC_Transmit_FS(upstream_buffer, stream.bytes_written + HeaderSize);
    }
    return status;
}

/**
 * Transmit breakpoint upstream to GUI
 * @param enabled
 */
void supplyTransmitBreakpoint(bool enabled)
{
    Supply_Breakpoint breakpoint;
    breakpoint.enabled = enabled;

    Supply_BreakpointStream datapacket;
    datapacket.which_packet = Supply_BreakpointStream_breakp_tag;
    datapacket.packet.breakp = breakpoint;

    Debugger_Upstream upstreamMsg = Debugger_Upstream_init_zero;

    upstreamMsg.which_packet = Debugger_Upstream_breakpoint_stream_tag;
    upstreamMsg.packet.breakpoint_stream = datapacket;

    transmitUpstreamPacket(upstreamMsg);
}

/**
 * Create upstream package with measurements to GUI
 */
void supplyTransmitRawIV(void){
    float vout = getDutVoltage();
    float ihigh = getDutHighCurrent();
    float ilow = getDutLowCurrent();
    float current = getDutCurrent();
    float vdebug = getDebugVoltage();

    Supply_DataStreamRawVI voltageCurrent;
    voltageCurrent.voltage = vout;
    voltageCurrent.current = current;
    voltageCurrent.has_current_high = true;
    voltageCurrent.current_high = ihigh;
    voltageCurrent.has_current_low = true;
    voltageCurrent.current_low = ilow;
    voltageCurrent.debug_voltage = vdebug;
    voltageCurrent.has_debug_voltage = true;

    Supply_DataStream datapacket;
    datapacket.which_packet = Supply_DataStream_volt_current_tag;
    datapacket.packet.volt_current = voltageCurrent;

    Debugger_Upstream upstreamMsg = Debugger_Upstream_init_zero;

    upstreamMsg.which_packet = Debugger_Upstream_datastream_tag;
    upstreamMsg.packet.datastream = datapacket;

    transmitUpstreamPacket(upstreamMsg);
}

/**
 * Create upstream status (mode) package
 */
void supplyTransmitStatus(void){
    Supply_OperationMode mode = getCurrentOperationMode();

    Supply_StatusStreamDTO statusStreamDto;
    statusStreamDto.status = mode;
    statusStreamDto.has_status = true;

    Supply_StatusStream datapacket;
    datapacket.which_packet = Supply_DataStream_volt_current_tag;
    datapacket.packet.status = statusStreamDto;

    Debugger_Upstream upstreamMsg = Debugger_Upstream_init_zero;

    upstreamMsg.which_packet = Debugger_Upstream_status_stream_tag;
    upstreamMsg.packet.status_stream = datapacket;

    transmitUpstreamPacket(upstreamMsg);
}

/**
 * Upstream request for new dataprofile information (replay mode)
 */
void supplyRequestDataProfile(void){
    Supply_ProfileDataRequest profileRequest;
    profileRequest.dummy_field = 1;

    Supply_ProfileDataRequestStream datapacket;
    datapacket.which_packet = Supply_ProfileDataRequestStream_data_tag;
    datapacket.packet.data = profileRequest;

    Debugger_Upstream upstreamMsg = Debugger_Upstream_init_zero;

    upstreamMsg.which_packet = Debugger_Upstream_profile_data_request_tag;
    upstreamMsg.packet.profile_data_request = datapacket;

    transmitUpstreamPacket(upstreamMsg);
}

/**
 * Transmit single parameter key and value (integers)
 * @param key
 * @param value <int>
 */
void supplyInitialParameterResponse_int(Supply_SupplyValueOption key, int value){
  Supply_InitialParameterResponse datapacket;
  datapacket.which_value = Supply_InitialParameterResponse_value_i_tag;
  datapacket.value.value_i = value;
  datapacket.key = key;

  Debugger_Upstream upstreamMsg = Debugger_Upstream_init_zero;

  upstreamMsg.which_packet = Debugger_Upstream_initial_param_response_tag;
  upstreamMsg.packet.initial_param_response = datapacket;

  transmitUpstreamPacket(upstreamMsg);
}

/**
 * Transmit single parameter key and value (float)
 * @param key
 * @param value <float>
 */
void supplyInitialParameterResponse_float(Supply_SupplyValueOption key, float value){
  Supply_InitialParameterResponse datapacket;
  datapacket.which_value = Supply_InitialParameterResponse_value_f_tag;
  datapacket.value.value_f = value;
  datapacket.key = key;

  Debugger_Upstream upstreamMsg = Debugger_Upstream_init_zero;

  upstreamMsg.which_packet = Debugger_Upstream_initial_param_response_tag;
  upstreamMsg.packet.initial_param_response = datapacket;

  transmitUpstreamPacket(upstreamMsg);
}

/**
 * Transmit single parameter key and value (bool)
 * @param key
 * @param value <bool>
 */
void supplyInitialParameterResponse_bool(Supply_SupplyValueOption key, bool value){
  Supply_InitialParameterResponse datapacket;
  datapacket.which_value = Supply_InitialParameterResponse_value_b_tag;
  datapacket.value.value_b = value;
  datapacket.key = key;

  Debugger_Upstream upstreamMsg = Debugger_Upstream_init_zero;

  upstreamMsg.which_packet = Debugger_Upstream_initial_param_response_tag;
  upstreamMsg.packet.initial_param_response = datapacket;

  transmitUpstreamPacket(upstreamMsg);
}
