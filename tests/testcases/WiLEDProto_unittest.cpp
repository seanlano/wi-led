/*
* WiLEDProto unit test program
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

#include "lib/WiLEDProto/WiLEDProto.h"
#include "lib/CRC16/CRC16.h"
#include "gtest/gtest.h"

// #define DO_LONG_TESTS // uncomment this to allow running long and slow tests

// Blank "storage read" function to pass to the WiLP initialiser
uint8_t BlankReader(uint16_t inAddress){
  uint16_t a = inAddress; // Just to avoid compiler warnings
  a++; // Just to avoid compiler warnings
  return 0;
}
// Blank "storage write" function to pass to the WiLP initialiser
void BlankWriter(uint16_t inAddress, uint8_t inValue){
  uint16_t a = inAddress; // Just to avoid compiler warnings
  a++; // Just to avoid compiler warnings
  uint8_t v = inValue; // Just to avoid compiler warnings
  v++; // Just to avoid compiler warnings
}
// Blank "storage commit" function to pass to the WiLP initialiser
void BlankCommitter(){
  uint8_t a = 0; // Just to avoid compiler warnings
  a++; // Just to avoid compiler warnings
}

/// Test fixture for WiLEDProto
class ProcessMessageTest : public testing::Test {
  protected:
    // Class instances within a class need to be initialised in the top-level
    // constructor - not in the declaration (hence this weird syntax)
    ProcessMessageTest() :
      p1(0x1000, &BlankReader, &BlankWriter, &BlankCommitter),
      p2(0x2000, &BlankReader, &BlankWriter, &BlankCommitter),
      p3(0x3000, &BlankReader, &BlankWriter, &BlankCommitter) {}

    // Declare some WiLP class hanlders to test
    WiLEDProto p1;
    WiLEDProto p2;
    WiLEDProto p3;

    // Set up the test fixture
    virtual void SetUp() {
      p1.initStorage();
      p2.initStorage();
      p3.initStorage();
    }

    //virtual void TearDown() {}
};

////////////////////////////////////////////////////////////////////////////////
// BEGIN tests for general message handling
////////////////////////////////////////////////////////////////////////////////

/// Test that the class is empty after being initialised
TEST_F(ProcessMessageTest, IsEmptyInitially) {
  EXPECT_EQ(p1.getLastReceivedResetCounter(), 0);
  EXPECT_EQ(p1.getLastReceivedMessageCounter(), 0);
  EXPECT_EQ(p1.getLastReceivedSource(), 0);
  EXPECT_EQ(p1.getLastReceivedDestination(), 0);
  EXPECT_EQ(p1.getLastReceivedType(), 0);
  EXPECT_EQ(p1.getLastReceivedMessageCounterValidation(), 0);
}

/// Test that the class correctly identifies an invalid message
TEST_F(ProcessMessageTest, DetectInvalidMessage) {
  uint8_t invalid_message[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Set magic number to 0x01 (i.e. invalid type)
  invalid_message[0] = 0x01;
  // Set source address to 0x1000 (big endian)
  invalid_message[1] = 0x10;
  invalid_message[2] = 0x00;
  // Set destination address to 0xFFFF
  invalid_message[3] = 0xFF;
  invalid_message[4] = 0xFF;

  // Check the message is processed and flagged as invalid
  ASSERT_EQ(WiLP_RETURN_INVALID_BUFFER, p1.processMessage(invalid_message));
  // Check the "getLast" calls are also set to zero after invalid message
  EXPECT_EQ(p1.getLastReceivedResetCounter(), 0);
  EXPECT_EQ(p1.getLastReceivedMessageCounter(), 0);
  EXPECT_EQ(p1.getLastReceivedSource(), 0);
  EXPECT_EQ(p1.getLastReceivedDestination(), 0);
  EXPECT_EQ(p1.getLastReceivedType(), 0);

  // Test another magic number
  invalid_message[0] = 0xFF;
  ASSERT_EQ(WiLP_RETURN_INVALID_BUFFER, p2.processMessage(invalid_message));
  // Check the "getLast" calls are also set to zero after invalid message
  EXPECT_EQ(p2.getLastReceivedResetCounter(), 0);
  EXPECT_EQ(p2.getLastReceivedMessageCounter(), 0);
  EXPECT_EQ(p2.getLastReceivedSource(), 0);
  EXPECT_EQ(p2.getLastReceivedDestination(), 0);
  EXPECT_EQ(p2.getLastReceivedType(), 0);
}

/// Check the class correctly sets its reset counter
TEST_F(ProcessMessageTest, CorrectFirstResetCounter) {
  // Create a buffer to store the output message
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Choose a value for "millis()" to pass to the sender
  const uint32_t p1_millis = 1234567890;
  // Process the message and copy it to a buffer
  p1.sendMessageBeacon(p1_millis);
  p1.copyToBuffer(p1_buffer);
  // Pull out the reset counter from the buffer
  uint16_t p1_reset_counter = 0;
  // Bytes 5 and 6 are the reset counter
  p1_reset_counter = (p1_buffer[5] << 8);
  p1_reset_counter += p1_buffer[6];
  // Reset counter should be 1 (since we just initialised p1)
  ASSERT_EQ(p1_reset_counter, 1);
}

/// Check the class correctly sets its message counter
TEST_F(ProcessMessageTest, CorrectFirstMessageCounter) {
  // Create a buffer to store the output message
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Choose a value for "millis()" to pass to the sender
  const uint32_t p1_millis = 1234567890;
  // Process the message and copy it to a buffer
  p1.sendMessageBeacon(p1_millis);
  p1.copyToBuffer(p1_buffer);
  // Pull out the message counter from the buffer
  uint16_t p1_message_counter = 0;
  // Bytes 7 and 8 are the message counter
  p1_message_counter = (p1_buffer[7] << 8);
  p1_message_counter += p1_buffer[8];
  // Reset counter should be 1 (since we just initialised p1)
  ASSERT_EQ(p1_message_counter, 1);

  // Pull out the reset counter from the buffer
  uint16_t p1_reset_counter = 0;
  // Bytes 5 and 6 are the reset counter
  p1_reset_counter = (p1_buffer[5] << 8);
  p1_reset_counter += p1_buffer[6];
  // Reset counter should be 1 (since we just initialised p1)
  ASSERT_EQ(p1_reset_counter, 1);
}

/// Check the class correctly sets its message counter
TEST_F(ProcessMessageTest, Correct254MessageCounter) {
  const uint16_t number_runs = 254;
  // Create a buffer to store the output message
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Choose a value for "millis()" to pass to the sender
  const uint32_t p1_millis = 1234567890;
  // Loop for 254 times, to increment the message counter
  for (uint16_t message_counter = 0; message_counter < number_runs; message_counter++)
  {
    p1.sendMessageBeacon(p1_millis);
    p1.copyToBuffer(p1_buffer);
  }

  // Pull out the message counter from the buffer
  uint16_t p1_message_counter = 0;
  // Bytes 7 and 8 are the message counter
  p1_message_counter = (p1_buffer[7] << 8);
  p1_message_counter += p1_buffer[8];
  // Do the actual comparison
  ASSERT_EQ(p1_message_counter, number_runs);

  // Pull out the reset counter from the buffer
  uint16_t p1_reset_counter = 0;
  // Bytes 5 and 6 are the reset counter
  p1_reset_counter = (p1_buffer[5] << 8);
  p1_reset_counter += p1_buffer[6];
  // Reset counter should be 1 (since we just initialised p1)
  ASSERT_EQ(p1_reset_counter, 1);
}

/// Check the class correctly sets its message counter
TEST_F(ProcessMessageTest, Correct1000MessageCounter) {
  const uint16_t number_runs = 1000;
  // Create a buffer to store the output message
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Choose a value for "millis()" to pass to the sender
  const uint32_t p1_millis = 1234567890;
  // Loop for 1000 times, to increment the message counter
  for (uint16_t message_counter = 0; message_counter < number_runs; message_counter++)
  {
    p1.sendMessageBeacon(p1_millis);
    p1.copyToBuffer(p1_buffer);
  }
  // Pull out the message counter from the buffer
  uint16_t p1_message_counter = 0;
  // Bytes 7 and 8 are the message counter
  p1_message_counter = (p1_buffer[7] << 8);
  p1_message_counter += p1_buffer[8];
  // Do the actual comparison
  ASSERT_EQ(p1_message_counter, number_runs);

  // Pull out the reset counter from the buffer
  uint16_t p1_reset_counter = 0;
  // Bytes 5 and 6 are the reset counter
  p1_reset_counter = (p1_buffer[5] << 8);
  p1_reset_counter += p1_buffer[6];
  // Reset counter should be 1 (since we just initialised p1)
  ASSERT_EQ(p1_reset_counter, 1);
}

/// Check the class correctly sets its message counter
TEST_F(ProcessMessageTest, Correct65535MessageCounter) {
  const uint16_t number_runs = 65535;
  // Create a buffer to store the output message
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Choose a value for "millis()" to pass to the sender
  const uint32_t p1_millis = 1234567890;
  // Loop for 65535 times, to increment the message counter
  for (uint16_t message_counter = 0; message_counter < number_runs; message_counter++)
  {
    p1.sendMessageBeacon(p1_millis);
    p1.copyToBuffer(p1_buffer);
  }
  // Pull out the message counter from the buffer
  uint16_t p1_message_counter = 0;
  // Bytes 7 and 8 are the message counter
  p1_message_counter = (p1_buffer[7] << 8);
  p1_message_counter += p1_buffer[8];
  // Do the actual comparison
  ASSERT_EQ(p1_message_counter, number_runs);

  // Pull out the reset counter from the buffer
  uint16_t p1_reset_counter = 0;
  // Bytes 5 and 6 are the reset counter
  p1_reset_counter = (p1_buffer[5] << 8);
  p1_reset_counter += p1_buffer[6];
  // Reset counter should be 1 (since we just initialised p1)
  ASSERT_EQ(p1_reset_counter, 1);
}

/// Check the class correctly sets its message counter (after overflow)
TEST_F(ProcessMessageTest, Correct65536MessageCounter) {
  const uint32_t number_runs = 65536;
  // Create a buffer to store the output message
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Choose a value for "millis()" to pass to the sender
  const uint32_t p1_millis = 1234567890;
  // Loop for 65536 times, to increment the message counter and oveflow it
  for (uint32_t message_counter = 0; message_counter < number_runs; message_counter++)
  {
    p1.sendMessageBeacon(p1_millis);
    p1.copyToBuffer(p1_buffer);
  }
  // Pull out the message counter from the buffer
  uint16_t p1_message_counter = 0;
  // Bytes 7 and 8 are the message counter
  p1_message_counter = (p1_buffer[7] << 8);
  p1_message_counter += p1_buffer[8];
  // Do the actual comparison (should have message_counter = 1 after overflow)
  EXPECT_EQ(p1_message_counter, 1);

  // Pull out the reset counter from the buffer
  uint16_t p1_reset_counter = 0;
  // Bytes 5 and 6 are the reset counter
  p1_reset_counter = (p1_buffer[5] << 8);
  p1_reset_counter += p1_buffer[6];
  // Reset counter should be 2 (since we just overflowed p1)
  EXPECT_EQ(p1_reset_counter, 2);
}

/// Check the class correctly sets its message counter (after overflow)
TEST_F(ProcessMessageTest, Correct65537MessageCounter) {
  const uint32_t number_runs = 65537;
  // Create a buffer to store the output message
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Choose a value for "millis()" to pass to the sender
  const uint32_t p1_millis = 1234567890;
  // Loop for 65537 times, to increment the message counter and oveflow it
  for (uint32_t message_counter = 0; message_counter < number_runs; message_counter++)
  {
    p1.sendMessageBeacon(p1_millis);
    p1.copyToBuffer(p1_buffer);
  }
  // Pull out the message counter from the buffer
  uint16_t p1_message_counter = 0;
  // Bytes 7 and 8 are the message counter
  p1_message_counter = (p1_buffer[7] << 8);
  p1_message_counter += p1_buffer[8];
  // Do the actual comparison (should have message_counter = 2, 1 run after overflow)
  EXPECT_EQ(p1_message_counter, 2);

  // Pull out the reset counter from the buffer
  uint16_t p1_reset_counter = 0;
  // Bytes 5 and 6 are the reset counter
  p1_reset_counter = (p1_buffer[5] << 8);
  p1_reset_counter += p1_buffer[6];
  // Reset counter should be 2 (since we overflowed p1)
  EXPECT_EQ(p1_reset_counter, 2);
}

#ifdef DO_LONG_TESTS
/// Check the class correctly sets its message counter (just before 2nd overflow)
TEST_F(ProcessMessageTest, Correct131070MessageCounter) {
  const uint32_t number_runs = 131070;
  // Create a buffer to store the output message
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Choose a value for "millis()" to pass to the sender
  const uint32_t p1_millis = 1234567890;
  // Loop for 65536 times, to increment the message counter and oveflow it
  for (uint32_t message_counter = 0; message_counter < number_runs; message_counter++)
  {
    p1.sendMessageBeacon(p1_millis);
    p1.copyToBuffer(p1_buffer);
  }
  // Pull out the message counter from the buffer
  uint16_t p1_message_counter = 0;
  // Bytes 7 and 8 are the message counter
  p1_message_counter = (p1_buffer[7] << 8);
  p1_message_counter += p1_buffer[8];
  // Do the actual comparison (message_counter = 65535 just before overflow)
  EXPECT_EQ(p1_message_counter, 65535);

  // Pull out the reset counter from the buffer
  uint16_t p1_reset_counter = 0;
  // Bytes 5 and 6 are the reset counter
  p1_reset_counter = (p1_buffer[5] << 8);
  p1_reset_counter += p1_buffer[6];
  // Reset counter should be 2 (since we overflowed p1 once)
  EXPECT_EQ(p1_reset_counter, 2);
}

/// Check the class correctly sets its message counter (just after 2nd overflow)
TEST_F(ProcessMessageTest, Correct131071MessageCounter) {
  const uint32_t number_runs = 131071;
  // Create a buffer to store the output message
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Choose a value for "millis()" to pass to the sender
  const uint32_t p1_millis = 1234567890;
  // Loop for 65536 times, to increment the message counter and oveflow it
  for (uint32_t message_counter = 0; message_counter < number_runs; message_counter++)
  {
    p1.sendMessageBeacon(p1_millis);
    p1.copyToBuffer(p1_buffer);
  }
  // Pull out the message counter from the buffer
  uint16_t p1_message_counter = 0;
  // Bytes 7 and 8 are the message counter
  p1_message_counter = (p1_buffer[7] << 8);
  p1_message_counter += p1_buffer[8];
  // Do the actual comparison (message_counter = 1 just after overflow)
  EXPECT_EQ(p1_message_counter, 1);

  // Pull out the reset counter from the buffer
  uint16_t p1_reset_counter = 0;
  // Bytes 5 and 6 are the reset counter
  p1_reset_counter = (p1_buffer[5] << 8);
  p1_reset_counter += p1_buffer[6];
  // Reset counter should be 3 (since we overflowed p1 twice)
  EXPECT_EQ(p1_reset_counter, 3);
}

/// Check the class sends and then another can receive, 4 million times
TEST_F(ProcessMessageTest, CorrectSendReceiveFourMillion) {
  // Create buffer for sent message
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};

  // Store the Beacon message type number
  const uint8_t beacon_type = WiLP_Beacon;

  // Run a loop many times
  const uint32_t loop_messages = 4000000;
  for (uint32_t loop = 0; loop < loop_messages; loop++){
    // "Send" message and copy to buffer
    uint32_t uptime = loop*2;
    p1.sendMessageBeacon(uptime);
    p1.copyToBuffer(p1_buffer);

    // Next, check the message from p1 is received properly by p2
    // These are all "assert" tests, so we don't print a million error messages
    ASSERT_EQ(p2.processMessage(p1_buffer), WiLP_RETURN_SUCCESS);
    // Check the "getLast" calls are also valid
    ASSERT_EQ(p2.getLastReceivedSource(), 0x1000); // p1 is 0x1000 address
    ASSERT_EQ(p2.getLastReceivedDestination(), 0xFFFF); // Beacon is a broadcast
    ASSERT_EQ(p2.getLastReceivedType(), beacon_type);
  }
}
#endif

// TODO: Test that reset counter is correctly sent to the "storage" callback
// TODO: Test that reset counter is correctly read from the "storage" callback
// TODO: Test that address array is correctly sent to the "storage" callback
// TODO: Test that address array is correctly read from the "storage" callback

/// Check the class correctly identifies a repeated message
TEST_F(ProcessMessageTest, DetectRepeatedMessage) {
  // First, create a message in p1 with uptime
  const uint32_t uptime = 0x499602D2; // Decimal = 1234567890
  // Create buffer for sent message
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};
  // "Send" message and copy to buffer
  p1.sendMessageBeacon(uptime);
  p1.copyToBuffer(p1_buffer);
  // Store the Beacon message type number
  const uint8_t beacon_type = WiLP_Beacon;

  // Next, check the message from p1 is received properly by p2
  ASSERT_EQ(p2.processMessage(p1_buffer), WiLP_RETURN_SUCCESS);
  // Then, check p2 detects a repeated message
  const uint8_t invalid_message_counter = WiLP_RETURN_INVALID_MSG_CTR;
  ASSERT_EQ(p2.processMessage(p1_buffer), invalid_message_counter);

  // Check the "getLast" calls are also valid
  EXPECT_EQ(p2.getLastReceivedResetCounter(), 1); //p1 was just initialised
  EXPECT_EQ(p2.getLastReceivedMessageCounter(), 1); //p1 was just initialised
  EXPECT_EQ(p2.getLastReceivedSource(), 0x1000); // p1 is 0x1000 address
  EXPECT_EQ(p2.getLastReceivedDestination(), 0xFFFF); // Beacon is a broadcast
  EXPECT_EQ(p2.getLastReceivedType(), beacon_type);
}

////////////////////////////////////////////////////////////////////////////////
// END tests for general message handling
////////////////////////////////////////////////////////////////////////////////
// BEGIN tests for "Beacon" message type, 0x01
////////////////////////////////////////////////////////////////////////////////

/// Check the class correctly receives a Beacon message
TEST_F(ProcessMessageTest, CorrectBeaconMessageReceive) {
  // First, manually create a correct beacon message
  uint8_t valid_message[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Set magic number to 0xAA (i.e. valid type)
  valid_message[0] = 0xAA;
  // Set source address to 0x1001 (big endian)
  valid_message[1] = 0x10;
  valid_message[2] = 0x01;
  // Set destination address to 0xFFFF
  valid_message[3] = 0xFF;
  valid_message[4] = 0xFF;
  // Set reset counter to 1 (this should be the first message being sent)
  valid_message[5] = 0x00;
  valid_message[6] = 0x01;
  // Set message counter to 1 (this should be the first message being sent)
  valid_message[7] = 0x00;
  valid_message[8] = 0x01;
  // Set message type flag to WiLP_Beacon
  const uint8_t beacon_type = 0x01;
  valid_message[9] = beacon_type;
  // Set the 4 payload bytes to a big-endian uptime number
  //const uint32_t uptime = 0x499602D2; // Decimal = 1234567890
  valid_message[10] = 0x49;
  valid_message[11] = 0x96;
  valid_message[12] = 0x02;
  valid_message[13] = 0xD2;
  // CRC-CCITT (XModem) checksum (big-endian), 0x64DB
  valid_message[14] = 0x64;
  valid_message[15] = 0xDB;

  // Check the message is received properly
  EXPECT_EQ(p1.processMessage(valid_message), WiLP_RETURN_SUCCESS);
  // Check the "getLast" calls are also valid
  EXPECT_EQ(p1.getLastReceivedResetCounter(), 1);
  EXPECT_EQ(p1.getLastReceivedMessageCounter(), 1);
  EXPECT_EQ(p1.getLastReceivedSource(), 0x1001);
  EXPECT_EQ(p1.getLastReceivedDestination(), 0xFFFF);
  EXPECT_EQ(p1.getLastReceivedType(), beacon_type);
}

/// Check the class correctly sends a Beacon message
TEST_F(ProcessMessageTest, CorrectBeaconMessageSend) {
  // First, manually create a correct beacon message in an array
  uint8_t valid_message[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Set magic number to 0xAA (i.e. valid type)
  valid_message[0] = 0xAA;
  // Set source address to 0x1000 (big endian) for instance p1
  valid_message[1] = 0x10;
  valid_message[2] = 0x00;
  // Set destination address to 0xFFFF
  valid_message[3] = 0xFF;
  valid_message[4] = 0xFF;
  // Set reset counter to 1 (this should be the first message being sent)
  valid_message[5] = 0x00;
  valid_message[6] = 0x01;
  // Set message counter to 1 (this should be the first message being sent)
  valid_message[7] = 0x00;
  valid_message[8] = 0x01;
  // Set message type flag to WiLP_Beacon
  const uint8_t beacon_type = 0x01;
  valid_message[9] = beacon_type;
  // Set the 4 payload bytes to a big-endian uptime number
  const uint32_t uptime = 0x499602D2; // Decimal = 1234567890
  valid_message[10] = 0x49;
  valid_message[11] = 0x96;
  valid_message[12] = 0x02;
  valid_message[13] = 0xD2;
  // CRC-CCITT (XModem) checksum (big-endian), 0x67AE
  valid_message[14] = 0x67;
  valid_message[15] = 0xAE;

  // Create buffer for sent message
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};
  // "Send" message and copy to buffer
  p1.sendMessageBeacon(uptime);
  p1.copyToBuffer(p1_buffer);

  // Create a string-ified version of the two buffers, to print out if necessary
  std::stringstream p1_buffer_str, valid_message_str;
  p1_buffer_str << "p1 buffer is: ";
  for (uint8_t i = 0; i < MAXIMUM_MESSAGE_LENGTH; i++){
    p1_buffer_str << std::hex << std::setfill('0') << std::setw(2) <<
      (unsigned short) p1_buffer[i] << " ";
  }
  p1_buffer_str << ". ";
  valid_message_str << "expected buffer: ";
  for (uint8_t i = 0; i < MAXIMUM_MESSAGE_LENGTH; i++){
    valid_message_str << std::hex << std::setfill('0') << std::setw(2) <<
      (unsigned short) valid_message[i] << " ";
  }

  // Check the sent buffer is what we expect it to be
  for(uint8_t idx = 0; idx<MAXIMUM_MESSAGE_LENGTH; idx++)
  {
    // Do the test, and print the actual and expected buffers if not equal
    ASSERT_EQ(p1_buffer[idx], valid_message[idx]) <<
      ::testing::PrintToString(p1_buffer_str.str() + valid_message_str.str());
  }
}

/// Check the class correctly sends and then another can receive a Beacon message
TEST_F(ProcessMessageTest, CorrectBeaconMessageSendReceive) {
  // First, create a message in p1 with uptime
  const uint32_t uptime = 0x499602D2; // Decimal = 1234567890
  // Create buffer for sent message
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};
  // "Send" message and copy to buffer
  p1.sendMessageBeacon(uptime);
  p1.copyToBuffer(p1_buffer);
  // Store the Beacon message type number
  const uint8_t beacon_type = WiLP_Beacon;

  // Next, check the message from p1 is received properly by p2
  ASSERT_EQ(p2.processMessage(p1_buffer), WiLP_RETURN_SUCCESS);
  // Check the "getLast" calls are also valid
  EXPECT_EQ(p2.getLastReceivedResetCounter(), 1); //p1 was just initialised
  EXPECT_EQ(p2.getLastReceivedMessageCounter(), 1); //p1 was just initialised
  EXPECT_EQ(p2.getLastReceivedSource(), 0x1000); // p1 is 0x1000 address
  EXPECT_EQ(p2.getLastReceivedDestination(), 0xFFFF); // Beacon is a broadcast
  EXPECT_EQ(p2.getLastReceivedType(), beacon_type);
}

/// Check the class correctly handles the callback for Beacon messages
// Define a callback to be used when a Beacon message is received
uint16_t p2_processed_address = 0;
uint32_t p2_processed_uptime = 0;
void handleBeacon(uint16_t sourceAddress, uint32_t uptime){
  p2_processed_address = sourceAddress;
  p2_processed_uptime = uptime;
}
TEST_F(ProcessMessageTest, CorrectBeaconMessageCallback) {
  // Reset these variables for this test, just in case
  p2_processed_address = 0;
  p2_processed_uptime = 0;
  // Create a message in p1 with uptime
  const uint32_t input_uptime = 0x499602D2; // Decimal = 1234567890
  const uint16_t input_address = 0x1000; // p1 address is 0x1000
  // Create buffer for sent message
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};
  // "Send" message and copy to buffer
  p1.sendMessageBeacon(input_uptime);
  p1.copyToBuffer(p1_buffer);

  // Configure p2 to use the 'handleBeacon' function as a callback
  p2.setCallbackBeacon(&handleBeacon);

  // Next, check the message from p1 is received properly by p2
  ASSERT_EQ(p2.processMessage(p1_buffer), WiLP_RETURN_SUCCESS);
  // Check the "getLast" calls are also valid
  EXPECT_EQ(p2.getLastReceivedSource(), input_address); // p1 is 0x1000 address
  // Check the callback is handled properly
  p2.handleLastMessage(); // This should set p2_processed_uptime to input_uptime
  EXPECT_EQ(p2_processed_address, input_address);
  EXPECT_EQ(p2_processed_uptime, input_uptime);
}
////////////////////////////////////////////////////////////////////////////////
// END tests for "Beacon" message type
////////////////////////////////////////////////////////////////////////////////
// BEGIN tests for "Set Individual" message type, 0x10
////////////////////////////////////////////////////////////////////////////////

/// Check the class correctly handles a valid 'Set Individual' message
uint8_t p1_output = 0;
uint8_t handleSetIndividual_hasrun = false;
void handleSetIndividual(WiLEDStatus inStatus){
  // Set the 'output' to the given level
  p1_output = inStatus.level;
  // Flag that the callback has been run, to detect possible erroneous calls
  handleSetIndividual_hasrun = true;
}
TEST_F(ProcessMessageTest, CorrectSetIndividualReceiveCallback) {
  // Reset the output
  p1_output = 0;
  handleSetIndividual_hasrun = false;
  // Create a valid message to pass to p1
  uint8_t valid_message[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Set magic number to 0xAA (i.e. valid type)
  valid_message[0] = 0xAA;
  // Set source address to 0x1111 (big endian)
  valid_message[1] = 0x11;
  valid_message[2] = 0x11;
  // Set destination address to 0xFFFF
  valid_message[3] = 0xFF;
  valid_message[4] = 0xFF;
  // Set reset counter to 1 (this should be the first message being sent)
  valid_message[5] = 0x00;
  valid_message[6] = 0x01;
  // Set message counter to 1 (this should be the first message being sent)
  valid_message[7] = 0x00;
  valid_message[8] = 0x01;
  // Set message type flag to WiLP_Set_Individual
  const uint8_t set_individual_type = WiLP_Set_Individual; // 0x10
  valid_message[9] = set_individual_type;
  // Set the 3 payload bytes to the output level and target address
  const uint8_t target_level = 0x64; // Decimal = 100
  valid_message[10] = target_level;
  // Set target address to 0x1000, i.e. address of p1
  valid_message[11] = 0x10;
  valid_message[12] = 0x00;
  // CRC-CCITT (XModem) checksum (big-endian), 0x0C73
  valid_message[13] = 0x0C;
  valid_message[14] = 0x73;

  // Configure p1 to use the callback defined above
  p1.setCallbackSetIndividual(&handleSetIndividual);
  // Next, check the message is received properly by p1
  ASSERT_EQ(p1.processMessage(valid_message), WiLP_RETURN_SUCCESS);
  // Check the "getLast" calls are also valid
  EXPECT_EQ(p1.getLastReceivedResetCounter(), 1);
  EXPECT_EQ(p1.getLastReceivedMessageCounter(), 1);
  EXPECT_EQ(p1.getLastReceivedSource(), 0x1111); // manually set to 0x1111
  EXPECT_EQ(p1.getLastReceivedDestination(), 0xFFFF); // Set Individual is a broadcast
  EXPECT_EQ(p1.getLastReceivedType(), set_individual_type);
  // Check the callback runs properly
  p1.handleLastMessage(); // This should set p1_output to 0x64
  EXPECT_EQ(p1_output, target_level);
}

/// Check the class correctly ignores an invalid 'Set Individual' message
/// i.e. this test intentionally has a target address of 0x1001, not 0x1000
TEST_F(ProcessMessageTest, InvalidSetIndividualReceive) {
  // Reset the output
  p1_output = 0;
  handleSetIndividual_hasrun = false;
  // Create an invalid message to pass to p1
  uint8_t invalid_message[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Set magic number to 0xAA (i.e. valid type)
  invalid_message[0] = 0xAA;
  // Set source address to 0x1111 (big endian)
  invalid_message[1] = 0x11;
  invalid_message[2] = 0x11;
  // Set destination address to 0xFFFF
  invalid_message[3] = 0xFF;
  invalid_message[4] = 0xFF;
  // Set reset counter to 1 (this should be the first message being sent)
  invalid_message[5] = 0x00;
  invalid_message[6] = 0x01;
  // Set message counter to 1 (this should be the first message being sent)
  invalid_message[7] = 0x00;
  invalid_message[8] = 0x01;
  // Set message type flag to WiLP_Set_Individual
  const uint8_t set_individual_type = WiLP_Set_Individual; // 0x10
  invalid_message[9] = set_individual_type;
  // Set the 3 payload bytes to the output level and target address
  const uint8_t target_level = 0x64; // Decimal = 100
  invalid_message[10] = target_level;
  // Set target address to 0x1001, i.e. not the address of p1
  invalid_message[11] = 0x10;
  invalid_message[12] = 0x01;
  // CRC-CCITT (XModem) checksum (big-endian), 0x1C52
  invalid_message[13] = 0x1C;
  invalid_message[14] = 0x52;

  // Configure p1 to use the callback defined above
  p1.setCallbackSetIndividual(&handleSetIndividual);
  // Next, check the message is received properly by p1
  ASSERT_EQ(p1.processMessage(invalid_message), WiLP_RETURN_SUCCESS);
  // Check the "getLast" calls are also valid
  EXPECT_EQ(p1.getLastReceivedResetCounter(), 1);
  EXPECT_EQ(p1.getLastReceivedMessageCounter(), 1);
  EXPECT_EQ(p1.getLastReceivedSource(), 0x1111); // manually set to 0x1111
  EXPECT_EQ(p1.getLastReceivedDestination(), 0xFFFF); // Set Individual is a broadcast
  EXPECT_EQ(p1.getLastReceivedType(), set_individual_type);
  // Check the callback runs properly
  p1.handleLastMessage();
  // The callback should not be called at all
  EXPECT_FALSE(handleSetIndividual_hasrun);
  EXPECT_EQ(p1_output, 0);
}

/// Check the class correctly creates a 'Set Individual' message
TEST_F(ProcessMessageTest, CorrectSetIndividualSend) {
  // Create a buffer that contains the expected output
  uint8_t expected_message[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Set magic number to 0xAA (i.e. valid type)
  expected_message[0] = 0xAA;
  // Set source address to 0x1000 (big endian)
  expected_message[1] = 0x10;
  expected_message[2] = 0x00;
  // Set destination address to 0xFFFF
  expected_message[3] = 0xFF;
  expected_message[4] = 0xFF;
  // Set reset counter to 1 (this should be the first message being sent)
  expected_message[5] = 0x00;
  expected_message[6] = 0x01;
  // Set message counter to 1 (this should be the first message being sent)
  expected_message[7] = 0x00;
  expected_message[8] = 0x01;
  // Set message type flag to WiLP_Set_Individual
  const uint8_t set_individual_type = WiLP_Set_Individual; // 0x10
  expected_message[9] = set_individual_type;
  // Set the 3 payload bytes to the output level and target address
  const uint8_t target_level = 0x64; // Decimal = 100
  expected_message[10] = target_level;
  // Set target address to 0x1000, i.e. address of p1
  expected_message[11] = 0x20;
  expected_message[12] = 0x00;
  // CRC-CCITT (XModem) checksum (big-endian), 0x87E7
  expected_message[13] = 0x87;
  expected_message[14] = 0xE7;

  // Send a message to 0x2000 with brightness of 100
  p1.sendMessageSetIndividual(target_level, 0x2000);

  // Create buffer for sent message and copy message into it
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};
  p1.copyToBuffer(p1_buffer);

  // Create a string-ified version of the two buffers, to print out if necessary
  std::stringstream p1_buffer_str, expected_message_str;
  p1_buffer_str << "p1 buffer is: ";
  for (uint8_t i = 0; i < MAXIMUM_MESSAGE_LENGTH; i++){
    p1_buffer_str << std::hex << std::setfill('0') << std::setw(2) <<
      (unsigned short) p1_buffer[i] << " ";
  }
  p1_buffer_str << ". ";
  expected_message_str << "expected buffer: ";
  for (uint8_t i = 0; i < MAXIMUM_MESSAGE_LENGTH; i++){
    expected_message_str << std::hex << std::setfill('0') << std::setw(2) <<
      (unsigned short) expected_message[i] << " ";
  }

  // Check the sent buffer is what we expect it to be
  for(uint8_t idx = 0; idx<MAXIMUM_MESSAGE_LENGTH; idx++)
  {
    // Do the test, and print the actual and expected buffers if not equal
    ASSERT_EQ(p1_buffer[idx], expected_message[idx]) <<
      ::testing::PrintToString(p1_buffer_str.str() + expected_message_str.str());
  }
}

/// Check the class correctly sends and then can receive a Set Individual message
TEST_F(ProcessMessageTest, CorrectSetIndividualSendReceive) {
  // First, create a message in p1 with target output and target address
  const uint8_t target_level = 0x64; // Decimal = 100

  // Create buffer for sent message
  uint8_t p1_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};

  // "Send" message and copy to buffer
  p1.sendMessageSetIndividual(target_level, 0x2000); // p2 address is 0x2000
  p1.copyToBuffer(p1_buffer);
  // Store the Beacon message type number
  const uint8_t beacon_type = WiLP_Set_Individual;

  // Next, check the message from p1 is received properly by p2
  ASSERT_EQ(p2.processMessage(p1_buffer), WiLP_RETURN_SUCCESS);
  // Check the "getLast" calls are also valid
  EXPECT_EQ(p2.getLastReceivedResetCounter(), 1); //p1 was just initialised
  EXPECT_EQ(p2.getLastReceivedMessageCounter(), 1); //p1 was just initialised
  EXPECT_EQ(p2.getLastReceivedSource(), 0x1000); // p1 is 0x1000 address
  EXPECT_EQ(p2.getLastReceivedDestination(), 0xFFFF); // Set Individual is broadcast
  EXPECT_EQ(p2.getLastReceivedType(), beacon_type);
}

////////////////////////////////////////////////////////////////////////////////
// END tests for "Set Individual" message type
////////////////////////////////////////////////////////////////////////////////
// BEGIN tests for "Set Two Individuals" message type, 0x11
////////////////////////////////////////////////////////////////////////////////

/// Check the class correctly handles a valid 'Set Two Individuals' message
uint8_t handleSetTwoIndividual_hasrun = false;
void handleSetTwoIndividuals(WiLEDStatus inStatus){
  // Set the 'output' to the given level
  p1_output = inStatus.level;
  // Flag that the callback has been run, to detect possible erroneous calls
  handleSetTwoIndividual_hasrun = true;
}
TEST_F(ProcessMessageTest, CorrectSetTwoIndividualsReceiveCallback) {
  // Reset the output
  p1_output = 0;
  handleSetTwoIndividual_hasrun = false;
  // Create a valid message to pass to p1
  uint8_t valid_message[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Set magic number to 0xAA (i.e. valid type)
  valid_message[0] = 0xAA;
  // Set source address to 0x1111 (big endian)
  valid_message[1] = 0x11;
  valid_message[2] = 0x11;
  // Set destination address to 0xFFFF
  valid_message[3] = 0xFF;
  valid_message[4] = 0xFF;
  // Set reset counter to 1 (this should be the first message being sent)
  valid_message[5] = 0x00;
  valid_message[6] = 0x01;
  // Set message counter to 1 (this should be the first message being sent)
  valid_message[7] = 0x00;
  valid_message[8] = 0x01;
  // Set message type flag to WiLP_Set_Individual
  const uint8_t set_individuals_type = WiLP_Set_Two_Individuals; // 0x11
  valid_message[9] = set_individuals_type;
  // Set the 5 payload bytes to the output level and target address
  const uint8_t target_level = 0x64; // Decimal = 100
  valid_message[10] = target_level;
  // Set target address 1 to 0x2000, i.e. address of p2
  valid_message[11] = 0x20;
  valid_message[12] = 0x00;
  // Set target address 2 to 0x1000, i.e. address of p1
  valid_message[13] = 0x10;
  valid_message[14] = 0x00;
  // CRC-CCITT (XModem) checksum (big-endian), 0x61A3
  valid_message[15] = 0x61;
  valid_message[16] = 0xA3;

  // Configure p1 to use the callback defined above
  p1.setCallbackSetIndividual(&handleSetTwoIndividuals);
  // Next, check the message is received properly by p1
  ASSERT_EQ(p1.processMessage(valid_message), WiLP_RETURN_SUCCESS);
  // Check the "getLast" calls are also valid
  EXPECT_EQ(p1.getLastReceivedResetCounter(), 1);
  EXPECT_EQ(p1.getLastReceivedMessageCounter(), 1);
  EXPECT_EQ(p1.getLastReceivedSource(), 0x1111); // manually set to 0x1111
  EXPECT_EQ(p1.getLastReceivedDestination(), 0xFFFF); // Set Individual is a broadcast
  EXPECT_EQ(p1.getLastReceivedType(), set_individuals_type);
  // Check the callback runs properly
  p1.handleLastMessage(); // This should set p1_output to 0x64
  EXPECT_EQ(p1_output, target_level);
}

/// Check the class correctly ignores an invalid 'Set Individual' message
/// i.e. this test intentionally has a target address of 0x1001, not 0x1000
TEST_F(ProcessMessageTest, InvalidSetTwoIndividualsReceive) {
  // Reset the output
  p1_output = 0;
  handleSetTwoIndividual_hasrun = false;
  // Create an invalid message to pass to p1
  uint8_t invalid_message[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Set magic number to 0xAA (i.e. valid type)
  invalid_message[0] = 0xAA;
  // Set source address to 0x1111 (big endian)
  invalid_message[1] = 0x11;
  invalid_message[2] = 0x11;
  // Set destination address to 0xFFFF
  invalid_message[3] = 0xFF;
  invalid_message[4] = 0xFF;
  // Set reset counter to 1 (this should be the first message being sent)
  invalid_message[5] = 0x00;
  invalid_message[6] = 0x01;
  // Set message counter to 1 (this should be the first message being sent)
  invalid_message[7] = 0x00;
  invalid_message[8] = 0x01;
  // Set message type flag to WiLP_Set_Individual
  const uint8_t set_individuals_type = WiLP_Set_Two_Individuals; // 0x11
  invalid_message[9] = set_individuals_type;
  // Set the 5 payload bytes to the output level and target address
  const uint8_t target_level = 0x64; // Decimal = 100
  invalid_message[10] = target_level;
  // Set target address 1 to 0x1001, i.e. not the address of p1
  invalid_message[11] = 0x10;
  invalid_message[12] = 0x01;
  // Set target address 2 to 0x5432, i.e. not the address of p1
  invalid_message[13] = 0x54;
  invalid_message[14] = 0x32;
  // CRC-CCITT (XModem) checksum (big-endian), 0xAD63
  invalid_message[15] = 0xAD;
  invalid_message[16] = 0x63;

  // Configure p1 to use the callback defined above
  p1.setCallbackSetIndividual(&handleSetTwoIndividuals);
  // Next, check the message is received properly by p1
  ASSERT_EQ(p1.processMessage(invalid_message), WiLP_RETURN_SUCCESS);
  // Check the "getLast" calls are also valid
  EXPECT_EQ(p1.getLastReceivedResetCounter(), 1);
  EXPECT_EQ(p1.getLastReceivedMessageCounter(), 1);
  EXPECT_EQ(p1.getLastReceivedSource(), 0x1111); // manually set to 0x1111
  EXPECT_EQ(p1.getLastReceivedDestination(), 0xFFFF); // Set Individual is a broadcast
  EXPECT_EQ(p1.getLastReceivedType(), set_individuals_type);
  // Check the callback runs properly
  p1.handleLastMessage();
  // The callback should not be called at all
  EXPECT_FALSE(handleSetTwoIndividual_hasrun);
  EXPECT_EQ(p1_output, 0);
}

/// Check the class correctly sends and then can receive a Set Two Individuals message
TEST_F(ProcessMessageTest, CorrectSetTwoIndividualsSendReceive) {
  // Reset the output
  p1_output = 0;

  // First, create a message in p2 with target output and target address
  const uint8_t target_level = 0x64; // Decimal = 100

  // Create buffer for sent message
  uint8_t p2_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};

  // "Send" message and copy to buffer
  p2.sendMessageSetTwoIndividuals(target_level, 0x9876, 0x1000); // p1 address is 0x1000
  p2.copyToBuffer(p2_buffer);
  // Store the Beacon message type number
  const uint8_t message_type = WiLP_Set_Two_Individuals;

  // Configure p1 to use the callback defined above
  p1.setCallbackSetIndividual(&handleSetTwoIndividuals);

  // Next, check the message from p2 is received properly by p1
  ASSERT_EQ(p1.processMessage(p2_buffer), WiLP_RETURN_SUCCESS);
  // Check the "getLast" calls are also valid
  EXPECT_EQ(p1.getLastReceivedResetCounter(), 1); //p2 was just initialised
  EXPECT_EQ(p1.getLastReceivedMessageCounter(), 1); //p2 was just initialised
  EXPECT_EQ(p1.getLastReceivedSource(), 0x2000); // p2 is 0x2000 address
  EXPECT_EQ(p1.getLastReceivedDestination(), 0xFFFF); // Set Individual is broadcast
  EXPECT_EQ(p1.getLastReceivedType(), message_type);
  // Check the callback runs properly
  p1.handleLastMessage(); // This should set p1_output to 0x64
  EXPECT_EQ(p1_output, target_level);
}
////////////////////////////////////////////////////////////////////////////////
// END tests for "Set Two Individuals" message type
////////////////////////////////////////////////////////////////////////////////
// BEGIN tests for "Set Three Individuals" message type, 0x12
////////////////////////////////////////////////////////////////////////////////

/// Check the class correctly handles a valid 'Set Three Individuals' message
uint8_t handleSetThreeIndividual_hasrun = false;
void handleSetThreeIndividuals(WiLEDStatus inStatus){
  // Set the 'output' to the given level
  p1_output = inStatus.level;
  // Flag that the callback has been run, to detect possible erroneous calls
  handleSetThreeIndividual_hasrun = true;
}
TEST_F(ProcessMessageTest, CorrectSetThreeIndividualsReceiveCallback) {
  // Reset the output
  p1_output = 0;
  handleSetThreeIndividual_hasrun = false;
  // Create a valid message to pass to p1
  uint8_t valid_message[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Set magic number to 0xAA (i.e. valid type)
  valid_message[0] = 0xAA;
  // Set source address to 0x1111 (big endian)
  valid_message[1] = 0x11;
  valid_message[2] = 0x11;
  // Set destination address to 0xFFFF
  valid_message[3] = 0xFF;
  valid_message[4] = 0xFF;
  // Set reset counter to 1 (this should be the first message being sent)
  valid_message[5] = 0x00;
  valid_message[6] = 0x01;
  // Set message counter to 1 (this should be the first message being sent)
  valid_message[7] = 0x00;
  valid_message[8] = 0x01;
  // Set message type flag to WiLP_Set_Individual
  const uint8_t set_individuals_type = WiLP_Set_Three_Individuals; // 0x12
  valid_message[9] = set_individuals_type;
  // Set the 7 payload bytes to the output level and target address
  const uint8_t target_level = 0x64; // Decimal = 100
  valid_message[10] = target_level;
  // Set target address 1 to 0x2000, i.e. address of p2
  valid_message[11] = 0x20;
  valid_message[12] = 0x00;
  // Set target address 2 to 0x3000, i.e. address of p3
  valid_message[13] = 0x30;
  valid_message[14] = 0x00;
  // Set target address 3 to 0x1000, i.e. address of p1
  valid_message[15] = 0x10;
  valid_message[16] = 0x00;
  // CRC-CCITT (XModem) checksum (big-endian), 0x41DA
  valid_message[17] = 0x41;
  valid_message[18] = 0xDA;

  // Configure p1 to use the callback defined above
  p1.setCallbackSetIndividual(&handleSetThreeIndividuals);
  // Next, check the message is received properly by p1
  ASSERT_EQ(p1.processMessage(valid_message), WiLP_RETURN_SUCCESS);
  // Check the "getLast" calls are also valid
  EXPECT_EQ(p1.getLastReceivedResetCounter(), 1);
  EXPECT_EQ(p1.getLastReceivedMessageCounter(), 1);
  EXPECT_EQ(p1.getLastReceivedSource(), 0x1111); // manually set to 0x1111
  EXPECT_EQ(p1.getLastReceivedDestination(), 0xFFFF); // Set Individual is a broadcast
  EXPECT_EQ(p1.getLastReceivedType(), set_individuals_type);
  // Check the callback runs properly
  p1.handleLastMessage(); // This should set p1_output to 0x64
  EXPECT_EQ(p1_output, target_level);
}

/// Check the class correctly ignores an invalid 'Set Three Individuals' message
/// i.e. this test intentionally has a target address of 0x1001, not 0x1000
TEST_F(ProcessMessageTest, InvalidSetThreeIndividualsReceive) {
  // Reset the output
  p1_output = 0;
  handleSetThreeIndividual_hasrun = false;
  // Create an invalid message to pass to p1
  uint8_t invalid_message[MAXIMUM_MESSAGE_LENGTH] = {0};
  // Set magic number to 0xAA (i.e. valid type)
  invalid_message[0] = 0xAA;
  // Set source address to 0x1111 (big endian)
  invalid_message[1] = 0x11;
  invalid_message[2] = 0x11;
  // Set destination address to 0xFFFF
  invalid_message[3] = 0xFF;
  invalid_message[4] = 0xFF;
  // Set reset counter to 1 (this should be the first message being sent)
  invalid_message[5] = 0x00;
  invalid_message[6] = 0x01;
  // Set message counter to 1 (this should be the first message being sent)
  invalid_message[7] = 0x00;
  invalid_message[8] = 0x01;
  // Set message type flag to WiLP_Set_Individual
  const uint8_t set_individuals_type = WiLP_Set_Three_Individuals; // 0x12
  invalid_message[9] = set_individuals_type;
  // Set the 5 payload bytes to the output level and target address
  const uint8_t target_level = 0x64; // Decimal = 100
  invalid_message[10] = target_level;
  // Set target address 1 to 0x1001, i.e. not the address of p1
  invalid_message[11] = 0x10;
  invalid_message[12] = 0x01;
  // Set target address 2 to 0x5432, i.e. not the address of p1
  invalid_message[13] = 0x54;
  invalid_message[14] = 0x32;
  // Set target address 3 to 0x3000, i.e. address of p3
  invalid_message[15] = 0x30;
  invalid_message[16] = 0x00;
  // CRC-CCITT (XModem) checksum (big-endian), 0xFA07
  invalid_message[17] = 0xFA;
  invalid_message[18] = 0x07;

  // Configure p1 to use the callback defined above
  p1.setCallbackSetIndividual(&handleSetThreeIndividuals);
  // Next, check the message is received properly by p1
  ASSERT_EQ(p1.processMessage(invalid_message), WiLP_RETURN_SUCCESS);
  // Check the "getLast" calls are also valid
  EXPECT_EQ(p1.getLastReceivedResetCounter(), 1);
  EXPECT_EQ(p1.getLastReceivedMessageCounter(), 1);
  EXPECT_EQ(p1.getLastReceivedSource(), 0x1111); // manually set to 0x1111
  EXPECT_EQ(p1.getLastReceivedDestination(), 0xFFFF); // Set Individual is a broadcast
  EXPECT_EQ(p1.getLastReceivedType(), set_individuals_type);
  // Check the callback runs properly
  p1.handleLastMessage();
  // The callback should not be called at all
  EXPECT_FALSE(handleSetThreeIndividual_hasrun);
  EXPECT_EQ(p1_output, 0);
}

/// Check the class correctly sends and then can receive a Set Three Individuals message
TEST_F(ProcessMessageTest, CorrectSetThreeIndividualsSendReceive) {
  // Reset the output
  p1_output = 0;

  // First, create a message in p2 with target output and target address
  const uint8_t target_level = 0x64; // Decimal = 100

  // Create buffer for sent message
  uint8_t p2_buffer[MAXIMUM_MESSAGE_LENGTH] = {0};

  // "Send" message and copy to buffer
  p2.sendMessageSetThreeIndividuals(target_level, 0x9876, 0x4567, 0x1000); // p1 address is 0x1000
  p2.copyToBuffer(p2_buffer);
  // Store the Beacon message type number
  const uint8_t message_type = WiLP_Set_Three_Individuals;

  // Configure p1 to use the callback defined above
  p1.setCallbackSetIndividual(&handleSetThreeIndividuals);

  // Next, check the message from p2 is received properly by p1
  ASSERT_EQ(p1.processMessage(p2_buffer), WiLP_RETURN_SUCCESS);
  // Check the "getLast" calls are also valid
  EXPECT_EQ(p1.getLastReceivedResetCounter(), 1); //p2 was just initialised
  EXPECT_EQ(p1.getLastReceivedMessageCounter(), 1); //p2 was just initialised
  EXPECT_EQ(p1.getLastReceivedSource(), 0x2000); // p2 is 0x2000 address
  EXPECT_EQ(p1.getLastReceivedDestination(), 0xFFFF); // Set Individual is broadcast
  EXPECT_EQ(p1.getLastReceivedType(), message_type);
  // Check the callback runs properly
  p1.handleLastMessage(); // This should set p1_output to 0x64
  EXPECT_EQ(p1_output, target_level);
}
////////////////////////////////////////////////////////////////////////////////
// END tests for "Set Three Individuals" message type
////////////////////////////////////////////////////////////////////////////////
