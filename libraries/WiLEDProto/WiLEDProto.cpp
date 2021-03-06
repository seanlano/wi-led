/*
* WiLEDProto class
* Part of the "WiLED" project, https://github.com/seanlano/WiLED
* A C++ class for creating and processing messages using the WiLED
* Protocol.
* Copyright (C) 2017 Sean Lanigan.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "WiLEDProto.h"

/************ Public methods *****************************/

// Initialise the WiLEDProto class with its address
WiLEDProto::WiLEDProto(
  uint16_t inAddress,
  uint8_t (*inStorageReadCB)(uint16_t),
  void (*inStorageWriteCB)(uint16_t, uint8_t),
  void (*inStorageCommitCB)(void)
){
  // Store callbacks
  __storage_read_callback = inStorageReadCB;
  __storage_write_callback = inStorageWriteCB;
  __storage_commit_callback = inStorageCommitCB;
  // Store the address, must not be 0 or 65535
  if(inAddress > 0 && inAddress < 65535){
    __address = inAddress;
  } else {
    __address = 0;
  }
  // Set magic number
  __outgoing_message_buffer[0] = 0xAA;
  // Set source address (as big-endian 2-byte number)
  __outgoing_message_buffer[1] = (__address >> 8);
  __outgoing_message_buffer[2] = (__address);
}


void WiLEDProto::initStorage(){
  // Read the addresses and reset counter arrays from storage
  if(__storage_commit_callback > 0 && __storage_write_callback > 0){
    // TODO: Check the return value of these
    __restoreFromStorage_uint16t(__address_array, STORAGE_ADDRESSES_LOCATION, sizeof(__address_array));
    __restoreFromStorage_uint16t(__reset_counter_array, STORAGE_RESET_LOCATION, sizeof(__reset_counter_array));
    __restoreFromStorage_uint16t(&__count_addresses, STORAGE_COUNT_LOCATION, sizeof(__count_addresses));
    __restoreFromStorage_uint16t(&__self_reset_counter, STORAGE_SELF_RESET_LOCATION, sizeof(__self_reset_counter));
    __self_reset_counter++;
    __addToStorage_uint16t(&__self_reset_counter, STORAGE_SELF_RESET_LOCATION, sizeof(__self_reset_counter));
    __storage_commit_callback();

    // Arduino-specific debug
    Serial.print("Loaded addresses: ");
    Serial.println(__count_addresses);
    Serial.print("Addresses: ");
    for(uint16_t idx=0; idx < __count_addresses; idx++){
      Serial.print(__address_array[idx], HEX);
      Serial.print(", ");
    }
    Serial.println();
    Serial.print("This device's reset counter: ");
    Serial.println(__self_reset_counter);
    Serial.println();
  }
}


// Process a received message
uint8_t WiLEDProto::processMessage(uint8_t* inBuffer){
  // Check if first byte is magic number
  if(inBuffer[0] != 0xAA){
    __last_received_destination = 0;
    __last_received_source = 0;
    __last_received_type = 0;
    __last_received_reset_counter = 0;
    __last_received_message_counter = 0;
    return WiLP_RETURN_INVALID_BUFFER;
  }
  // Store the received message destination (left shift)
  __last_received_destination = (inBuffer[3] << 8);
  __last_received_destination += inBuffer[4];
  // Store the received message source (left shift)
  __last_received_source = (inBuffer[1] << 8);
  __last_received_source += inBuffer[2];
  // Store the received message type byte
  __last_received_type = inBuffer[9];
  // Store the received reset counter (left shift)
  __last_received_reset_counter = (inBuffer[5] << 8);
  __last_received_reset_counter += inBuffer[6];
  // Store the received message counter
  __last_received_message_counter = (inBuffer[7] << 8);
  __last_received_message_counter += inBuffer[8];
  // Check if we are the destination
  if((__last_received_destination != __address) and (__last_received_destination != 0xFFFF)){
    return WiLP_RETURN_NOT_THIS_DEST;
  }

  __last_received_message_counter_validation = __checkAndUpdateMessageCounter(__last_received_source, __last_received_reset_counter, __last_received_message_counter);

  // Determine the payload length
  switch(__last_received_type){
    case WiLP_Beacon:
      __last_received_payload_length = 4;
      break;
    default:
      __last_received_payload_length = 0;
      return WiLP_RETURN_UNKNOWN_TYPE;
  }
  // Store the payload bytes in an array
  if(__last_received_payload_length > 0){
    memcpy(__last_received_payload, &inBuffer[10], __last_received_payload_length);
  }

  return WiLP_RETURN_SUCCESS;
}


// Send a "Beacon" message
uint8_t WiLEDProto::sendMessageBeacon(uint32_t inUptime){
  __setTypeByte(WiLP_Beacon);
  __setDestinationByte(0xFFFF);

  // Use right-shift to break into four (big endian) 1-byte blocks
  __setPayloadByte(0, (inUptime >> 24));
  __setPayloadByte(1, (inUptime >> 16));
  __setPayloadByte(2, (inUptime >> 8));
  __setPayloadByte(3, (inUptime));

  return WiLP_RETURN_SUCCESS;
}


void WiLEDProto::copyToBuffer(uint8_t * inBuffer){
  /// copyToBuffer can only be called once. After calling, the
  /// message contents must be set again.
  // Set reset counter bytes
  __outgoing_message_buffer[5] = (__self_reset_counter >> 8);
  __outgoing_message_buffer[6] = (__self_reset_counter);

  // Increment and set message counter bytes (big endian)
  // TODO: Handle overflow of message counter
  __self_message_counter++;
  __outgoing_message_buffer[7] = (__self_message_counter >> 8);
  __outgoing_message_buffer[8] = (__self_message_counter);

  // Copy internal buffer to provided address
  memcpy(inBuffer, __outgoing_message_buffer, MAXIMUM_MESSAGE_LENGTH);

  // Wipe message buffer (keep magic number and source address)
  for(uint8_t idx = 3; idx<MAXIMUM_MESSAGE_LENGTH; idx++){
    __outgoing_message_buffer[idx] = 0x00;
  }
}


uint8_t WiLEDProto::getLastReceivedType(){
  return __last_received_type;
}

uint16_t WiLEDProto::getLastReceivedSource(){
  return __last_received_source;
}

uint16_t WiLEDProto::getLastReceivedDestination(){
  return __last_received_destination;
}

uint16_t WiLEDProto::getLastReceivedResetCounter(){
  return __last_received_reset_counter;
}

uint16_t WiLEDProto::getLastReceivedMessageCounter(){
  return __last_received_message_counter;
}

uint8_t WiLEDProto::getLastReceivedMessageCounterValidation(){
  return __last_received_message_counter_validation;
}

/************ Private methods ***************************/

// Set the "type" byte in the output buffer
void WiLEDProto::__setTypeByte(uint8_t inType){
  __outgoing_message_buffer[9] = inType;
}


// Set the "destination" bytes in the output buffer
void WiLEDProto::__setDestinationByte(uint16_t inDestination){
  // Set destination address (as big-endian 2-byte number)
  __outgoing_message_buffer[3] = (inDestination >> 8);
  __outgoing_message_buffer[4] = (inDestination);
}


// Set the specified "payload" byte in the output buffer
void WiLEDProto::__setPayloadByte(uint8_t inPayloadOffset, uint8_t inPayloadValue){
  __outgoing_message_buffer[10+inPayloadOffset] = inPayloadValue;
}


uint8_t WiLEDProto::__restoreFromStorage_uint16t(uint16_t* outArray, uint16_t inStorageOffset, uint16_t inLength){
  /*Serial.println("Reading to storage: ");
  Serial.println((int)(void*)outArray, HEX);
  Serial.println(inStorageOffset);
  Serial.println(inLength);*/
  // Make a 1-byte pointer to the array of 2-byte values
  uint8_t* p = (uint8_t*)(void*)outArray;
  // First, check callback has been set
  if(__storage_read_callback > 0){
    for (uint16_t idx = 0; idx < inLength; idx++){
      // Read from the storage location into the array
      p[idx] = (*__storage_read_callback)(idx + inStorageOffset);
    }
    return WiLP_RETURN_SUCCESS;
  } else {
    // If callback not set, return with an error
    return WiLP_RETURN_NOT_INIT;
  }
}


uint8_t WiLEDProto::__addToStorage_uint16t(uint16_t* inArray, uint16_t inStorageOffset, uint16_t inLength){
  /*Serial.println("Adding to storage: ");
  Serial.println((int)(void*)inArray, HEX);
  Serial.println(*inArray);
  Serial.println(inStorageOffset);
  Serial.println(inLength);*/
  // Make a 1-byte pointer to the array of 2-byte values
  uint8_t* p = (uint8_t*)(void*)inArray;
  // First, check callback has been set
  if(__storage_write_callback > 0){
    for (uint16_t idx = 0; idx < inLength; idx++){
      // Write from the array into the storage location
      (*__storage_write_callback)(idx + inStorageOffset, p[idx]);
    }
    //(*__storage_commit_callback)();
    return WiLP_RETURN_SUCCESS;
  } else {
    // If callback not set, return with an error
    return WiLP_RETURN_NOT_INIT;
  }
}


uint8_t WiLEDProto::__checkAndUpdateMessageCounter(uint16_t inAddress, uint16_t inResetCounter, uint16_t inMessageCounter){
  // Loop over all the known stored addresses
  for(uint16_t idx = 0; idx < __count_addresses; idx++){
    // Look for the requested address
    if(__address_array[idx] == inAddress){
      // Stored reset counter must be less than or equal to the current counter
      if(__reset_counter_array[idx] < inResetCounter){
        // If less than current value, save new value and reset message counter
        __reset_counter_array[idx] = inResetCounter;
        __addToStorage_uint16t(__address_array, STORAGE_ADDRESSES_LOCATION, sizeof(__address_array));
        __storage_commit_callback();
        __message_counter_array[idx] = inMessageCounter;
        // Message is fully valid so return success
        return WiLP_RETURN_SUCCESS;
      } else if (__reset_counter_array[idx] != inResetCounter){
        // Valid but no update needed, still check message counter
        return WiLP_RETURN_INVALID_RST_CTR;
      }
      // Stored message counter must be less than the current counter
      if(__message_counter_array[idx] < inMessageCounter){
        // If valid, update the stored message counter
        __message_counter_array[idx] = inMessageCounter;
        return WiLP_RETURN_SUCCESS;
      }
      else {
        // Message is invalid, ignore it
        return WiLP_RETURN_INVALID_MSG_CTR;
      }
    }
  }
  // If we reach this point, we did not previously know the address
  // Add the address to our known addresses
  if(__count_addresses < MAXIMUM_STORED_ADDRESSES){
    __address_array[__count_addresses] = inAddress;
    __message_counter_array[__count_addresses] = inMessageCounter;
    // Increment the counter
    __count_addresses++;
    // Save the new __address_array and __count_addresses to storage
    // TODO: Check return value of these
    __addToStorage_uint16t(__address_array, STORAGE_ADDRESSES_LOCATION, sizeof(__address_array));
    __addToStorage_uint16t(&__count_addresses, STORAGE_COUNT_LOCATION, sizeof(__count_addresses));
    __storage_commit_callback();
    return WiLP_RETURN_ADDED_ADDRESS;
  } else {
    // At maximum known addresses!
    return WiLP_RETURN_AT_MAX_ADDRESSES;
  }
  // We should never get here, but just in case
  return WiLP_RETURN_OTHER_ERROR;
}
