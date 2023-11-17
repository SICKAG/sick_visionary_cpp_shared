//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "CoLa2ProtocolHandler.h"
#include "CoLaParameterReader.h"
#include "CoLaParameterWriter.h"
#include "MockTransport.h"
#include "VisionaryEndian.h"
#include "gtest/gtest.h"

using namespace visionary_test;
using namespace visionary;
//---------------------------------------------------------------------------------------

TEST(CoLa2ProtocolHandlerTest, OpenSession)
{
  constexpr uint32_t   SESSION_ID = 0x4e11ba11u;
  MockCoLa2Transport   transport;
  CoLa2ProtocolHandler protocolHandler{transport};
  transport.sessionId(SESSION_ID).cmdMode("OA");

  EXPECT_TRUE(protocolHandler.openSession(50u));

  EXPECT_TRUE(transport.cmdHeader().has_value());
  EXPECT_EQ(transport.cmdHeader()->cmdMode, "Ox");
  EXPECT_EQ(protocolHandler.getSessionId(), SESSION_ID);
}

TEST(CoLa2ProtocolHandlerTest, ReadVariable)
{
  MockCoLa2Transport transport;
  const ByteBuffer   results({0x01u, 0x02u, 0x03u, 0x04u});
  transport.cmdMode("RA").name(" varname ").returnvals(results);

  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "varname").build();

  auto response = protocolHandler.send(testCommand);

  EXPECT_TRUE(transport.cmdHeader().has_value());
  EXPECT_EQ(transport.cmdHeader()->cmdMode, "RN");
  EXPECT_EQ(CoLaCommandType::READ_VARIABLE_RESPONSE, response.getType());
  EXPECT_EQ(CoLaError::OK, response.getError());

  EXPECT_EQ(CoLaParameterReader(response).readUDInt(), 0x01020304u);
}

TEST(CoLa2ProtocolHandlerTest, WriteVariable)
{
  MockCoLa2Transport transport;
  constexpr int32_t  VAR_VALUE = -0x12345678; // is hex 0xEDCBA988
  transport.cmdMode("WA").name(" vname ");

  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand =
    CoLaParameterWriter(CoLaCommandType::WRITE_VARIABLE, "vname").parameterDInt(VAR_VALUE).build();
  auto response = protocolHandler.send(testCommand);

  EXPECT_TRUE(transport.cmdHeader().has_value());
  EXPECT_EQ(transport.cmdHeader()->cmdMode, "WN");

  const ByteBuffer expectedValue{' ', 'v', 'n', 'a', 'm', 'e', ' ', 0xedu, 0xcbu, 0xa9u, 0x88u}; // varname + parameter
  EXPECT_EQ(transport.cmdpayload(), expectedValue);

  EXPECT_EQ(CoLaCommandType::WRITE_VARIABLE_RESPONSE, response.getType());
  EXPECT_EQ(CoLaError::OK, response.getError());
}

TEST(CoLa2ProtocolHandlerTest, MethodInvocation)
{
  MockCoLa2Transport transport;
  constexpr uint16_t PAR_VALUE = 0xfeed;
  const ByteBuffer   results({0x01u, 0x02u, 0x03u, 0x04u});
  transport.cmdMode("AN").name(" mtd ").returnvals(results);

  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand =
    CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "mtd").parameterUInt(PAR_VALUE).build();

  auto response = protocolHandler.send(testCommand);

  const ByteBuffer expectedValue({' ', 'm', 't', 'd', ' ', 0xfeu, 0xedu});
  EXPECT_EQ(transport.cmdpayload(), expectedValue);

  EXPECT_TRUE(transport.cmdHeader().has_value());
  EXPECT_EQ(transport.cmdHeader()->cmdMode, "MN");

  EXPECT_EQ(CoLaCommandType::METHOD_RETURN_VALUE, response.getType());
  EXPECT_EQ(CoLaError::OK, response.getError());

  EXPECT_EQ(CoLaParameterReader(response).readUDInt(), 0x01020304u);
}

TEST(CoLa2ProtocolHandlerTest, OpenSessionInvalidMagic)
{
  MockTransport        transport{0x02, 0x02, 0x02, 0x01};
  CoLa2ProtocolHandler protocolHandler{transport};

  EXPECT_FALSE(protocolHandler.openSession(50u));
}

TEST(CoLa2ProtocolHandlerTest, OpenSessionBrokenPacket)
{
  MockTransport        transport{0x02, 0x02, 0x02, 0x02, 0x0, 0x0, 0x0, 0x1, 0x1};
  CoLa2ProtocolHandler protocolHandler{transport};

  EXPECT_FALSE(protocolHandler.openSession(50u));
}

TEST(CoLa2ProtocolHandlerTest, OpenSessionEmptyPacket)
{
  MockTransport        transport;
  CoLa2ProtocolHandler protocolHandler{transport};

  EXPECT_FALSE(protocolHandler.openSession(50u));
}

TEST(CoLa2ProtocolHandlerTest, InvalidMagicBytes)
{
  MockTransport        transport{0x02, 0x02, 0x02, 0x01};
  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "varname").build();

  auto response = protocolHandler.send(testCommand);

  EXPECT_EQ(CoLaCommandType::NETWORK_ERROR, response.getType());
  EXPECT_EQ(CoLaError::NETWORK_ERROR, response.getError());
}

TEST(CoLa2ProtocolHandlerTest, tooFewMagicBytes)
{
  MockTransport        transport{0x02, 0x02, 0x02};
  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "varname").build();
  auto        response    = protocolHandler.send(testCommand);

  EXPECT_EQ(CoLaCommandType::NETWORK_ERROR, response.getType());
  EXPECT_EQ(CoLaError::NETWORK_ERROR, response.getError());
}

TEST(CoLa2ProtocolHandlerTest, EmptyAnswer)
{
  MockTransport        transport{0x02, 0x02, 0x02, 0x02};
  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "varname").build();
  auto        response    = protocolHandler.send(testCommand);

  EXPECT_EQ(CoLaCommandType::NETWORK_ERROR, response.getType());
  EXPECT_EQ(CoLaError::NETWORK_ERROR, response.getError());
}

TEST(CoLa2ProtocolHandlerTest, EmptyPackage)
{
  MockTransport        transport{0x02, 0x02, 0x02, 0x02};
  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "varname").build();
  auto        response    = protocolHandler.send(testCommand);

  EXPECT_EQ(CoLaCommandType::NETWORK_ERROR, response.getType());
  EXPECT_EQ(CoLaError::NETWORK_ERROR, response.getError());
}

TEST(CoLa2ProtocolHandlerTest, InvalidSessionId)
{
  MockCoLa2Transport transport;
  const ByteBuffer   results({0x01u, 0x02u, 0x03u, 0x04u});
  transport.sessionId(0xbadfeed1u).cmdMode("RA").name(" varname ").returnvals(results);

  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "varname").build();
  auto        response    = protocolHandler.send(testCommand);

  EXPECT_EQ(CoLaCommandType::NETWORK_ERROR, response.getType());
  EXPECT_EQ(CoLaError::NETWORK_ERROR, response.getError());
}

TEST(CoLa2ProtocolHandlerTest, InvalidReqId)
{
  MockCoLa2Transport transport;
  constexpr int32_t  VAR_VALUE = -0x12345678; // is hex 0xEDCBA988
  transport.reqId(0xdeadu).cmdMode("WA").name(" vname ");

  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand =
    CoLaParameterWriter(CoLaCommandType::WRITE_VARIABLE, "vname").parameterDInt(VAR_VALUE).build();
  auto response = protocolHandler.send(testCommand);

  EXPECT_EQ(CoLaCommandType::NETWORK_ERROR, response.getType());
  EXPECT_EQ(CoLaError::NETWORK_ERROR, response.getError());
}

TEST(CoLa2ProtocolHandlerTest, InvalidResponseCode)
{
  MockCoLa2Transport transport;
  transport.cmdMode("FB");
  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "varname").build();

  auto response = protocolHandler.send(testCommand);

  EXPECT_EQ(CoLaCommandType::UNKNOWN, response.getType());
  EXPECT_EQ(CoLaError::UNKNOWN, response.getError());
}

TEST(CoLa2ProtocolHandlerTest, ColaAnswerTooShort)
{
  MockCoLa2Transport transport;
  transport.cmdMode("RA");
  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "varname").build();

  auto response = protocolHandler.send(testCommand);

  EXPECT_EQ(CoLaCommandType::UNKNOWN, response.getType());
  EXPECT_EQ(CoLaError::UNKNOWN, response.getError());
}

TEST(CoLa2ProtocolHandlerTest, ColaErrorTooShort)
{
  MockCoLa2Transport transport;
  transport.cmdMode("F");
  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "varname").build();

  auto response = protocolHandler.send(testCommand);

  EXPECT_EQ(CoLaCommandType::UNKNOWN, response.getType());
  EXPECT_EQ(CoLaError::UNKNOWN, response.getError());
}

TEST(CoLa2ProtocolHandlerTest, ColaErrorErrornoMissing)
{
  MockCoLa2Transport transport;
  transport.cmdMode("FA").returnvals({0x01});
  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "varname").build();

  auto response = protocolHandler.send(testCommand);

  EXPECT_EQ(CoLaCommandType::UNKNOWN, response.getType());
  EXPECT_EQ(CoLaError::UNKNOWN, response.getError());
}

TEST(CoLa2ProtocolHandlerTest, ColaErrorErrorTooShort)
{
  MockCoLa2Transport transport;
  transport.cmdMode("FA");

  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "varname").build();

  auto response = protocolHandler.send(testCommand);

  EXPECT_EQ(CoLaCommandType::UNKNOWN, response.getType());
  EXPECT_EQ(CoLaError::UNKNOWN, response.getError());
}

TEST(CoLa2ProtocolHandlerTest, ColaErrorValid)
{
  MockCoLa2Transport transport;
  transport.cmdMode("FA").returnvals({0x00u, 0x04u});

  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "varname").build();

  auto response = protocolHandler.send(testCommand);

  EXPECT_EQ(CoLaCommandType::COLA_ERROR, response.getType());
  EXPECT_EQ(CoLaError::LOCAL_CONDITION_FAILED, response.getError());
}

TEST(CoLa2ProtocolHandlerTest, SendFailed)
{
  MockCoLa2Transport transport;
  transport.cmdMode("FA").returnvals({0x00u, 0x01u}).fakeSendReturn(-1);

  CoLa2ProtocolHandler protocolHandler{transport};

  CoLaCommand testCommand = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "varname").build();

  auto response = protocolHandler.send(testCommand);

  EXPECT_EQ(CoLaCommandType::NETWORK_ERROR, response.getType());
  EXPECT_EQ(CoLaError::NETWORK_ERROR, response.getError());
}
