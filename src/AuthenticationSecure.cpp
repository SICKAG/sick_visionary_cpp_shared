//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "AuthenticationSecure.h"
#include "CoLaParameterWriter.h"
#include "SHA256.h"

namespace visionary {

namespace {
enum class ChallengeResponseResult : std::uint8_t
{
  SUCCESS           = 0u,
  INVALID_CLIENT    = 1u,
  NOT_ACCEPTED      = 2u,
  UNKNOWN_CHALLENGE = 3u,
  PWD_NOT_CHANGABLE = 4u,
  TIMELOCK_ACTIVE   = 5u
};
}

AuthenticationSecure::AuthenticationSecure(VisionaryControl& vctrl) : m_VisionaryControl(vctrl), m_protocolType(UNKNOWN)

{
}

AuthenticationSecure::~AuthenticationSecure() = default;

PasswordHash AuthenticationSecure::CreatePasswordHash(UserLevel               userLevel,
                                                      const std::string&      password,
                                                      const ChallengeRequest& challengeRequest,
                                                      ProtocolType            protocolType)
{
  PasswordHash passwordHash{};
  std::string  passwordPrefix{};

  switch (userLevel)
  {
    case UserLevel::RUN:
    {
      passwordPrefix = "Run";
      break;
    }
    case UserLevel::OPERATOR:
    {
      passwordPrefix = "Operator";
      break;
    }
    case UserLevel::MAINTENANCE:
    {
      passwordPrefix = "Maintenance";
      break;
    }
    case UserLevel::AUTHORIZED_CLIENT:
    {
      passwordPrefix = "AuthorizedClient";
      break;
    }
    case UserLevel::SERVICE:
    {
      passwordPrefix = "Service";
      break;
    }
    default:
    {
      // return empty hash code in case of error
      return passwordHash;
      break;
    }
  }
  std::string separator          = ":";
  std::string passwordWithPrefix = passwordPrefix + ":SICK Sensor:" + password;

  hash_state hashState{};
  sha256_init(&hashState);

  sha256_process(&hashState,
                 reinterpret_cast<const uint8_t*>(passwordWithPrefix.c_str()),
                 static_cast<ulong32>(passwordWithPrefix.size()));
  if (protocolType == SUL2)
  {
    sha256_process(
      &hashState, reinterpret_cast<const uint8_t*>(separator.c_str()), static_cast<ulong32>(separator.size()));
    sha256_process(&hashState, challengeRequest.salt.data(), static_cast<ulong32>(challengeRequest.salt.size()));
  }
  sha256_done(&hashState, passwordHash.data());

  return passwordHash;
}

ChallengeResponse AuthenticationSecure::CreateChallengeResponse(UserLevel               userLevel,
                                                                const std::string&      password,
                                                                const ChallengeRequest& challengeRequest,
                                                                ProtocolType            protocolType)
{
  ChallengeResponse challengeResponse{};
  PasswordHash      passwordHash = CreatePasswordHash(userLevel, password, challengeRequest, protocolType);

  hash_state hashState{};
  sha256_init(&hashState);
  sha256_process(&hashState, passwordHash.data(), static_cast<ulong32>(passwordHash.size()));
  sha256_process(
    &hashState, challengeRequest.challenge.data(), static_cast<ulong32>(challengeRequest.challenge.size()));
  sha256_done(&hashState, challengeResponse.data());

  return challengeResponse;
}

bool AuthenticationSecure::loginImpl(UserLevel                  userLevel,
                                     const std::string&         password,
                                     const CoLaParameterReader& getChallengeResponse,
                                     ProtocolType               protocolType)
{
  bool isLoginSuccessful = false;
  // read and check response of GetChallenge command
  CoLaParameterReader coLaParameterReader = CoLaParameterReader(getChallengeResponse);
  if (static_cast<ChallengeResponseResult>(coLaParameterReader.readUSInt()) == ChallengeResponseResult::SUCCESS)
  {
    ChallengeRequest challengeRequest{};
    for (std::uint32_t byteCounter = 0u; byteCounter < sizeof(challengeRequest.challenge); byteCounter++)
    {
      challengeRequest.challenge[byteCounter] = coLaParameterReader.readUSInt();
    }
    if (protocolType == SUL2)
    {
      for (std::uint32_t byteCounter = 0u; byteCounter < sizeof(challengeRequest.salt); byteCounter++)
      {
        challengeRequest.salt[byteCounter] = coLaParameterReader.readUSInt();
      }
    }

    ChallengeResponse challengeResponse = CreateChallengeResponse(userLevel, password, challengeRequest, protocolType);

    CoLaParameterWriter coLaParameterWriter = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "SetUserLevel");

    // add challenge response value to set user level command
    for (unsigned char b : challengeResponse)
    {
      coLaParameterWriter.parameterUSInt(b);
    }

    // add user Level to command and build it
    CoLaCommand getUserLevelCommand  = coLaParameterWriter.parameterUSInt(static_cast<uint8_t>(userLevel)).build();
    CoLaCommand getUserLevelResponse = m_VisionaryControl.sendCommand(getUserLevelCommand);
    if (getUserLevelResponse.getError() == CoLaError::OK)
    {
      coLaParameterReader = CoLaParameterReader(getUserLevelResponse);
      if (static_cast<ChallengeResponseResult>(coLaParameterReader.readUSInt()) == ChallengeResponseResult::SUCCESS)
      {
        isLoginSuccessful = true;
      }
    }
    m_protocolType = protocolType;
  }
  return isLoginSuccessful;
}

bool AuthenticationSecure::login(UserLevel userLevel, const std::string& password)
{
  bool isLoginSuccessful{false};

  // create command to get the challenge
  CoLaParameterWriter getChallengeCommandBuilder =
    CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "GetChallenge");
  if (m_protocolType == UNKNOWN || m_protocolType == SUL1)
  {
    // send command and get the response
    CoLaCommand getChallengeResponse = m_VisionaryControl.sendCommand(getChallengeCommandBuilder.build());
    // check whether there occurred an error with the CoLa communication
    const auto errorCode = getChallengeResponse.getError();
    if (errorCode == CoLaError::OK)
    {
      isLoginSuccessful = loginImpl(userLevel, password, getChallengeResponse, SUL1);
    }
    else if (errorCode == CoLaError::BUFFER_UNDERFLOW)
    {
      m_protocolType = SUL2;
    }
  }

  if (m_protocolType == SUL2)
  {
    CoLaCommand getChallengeCommand =
      getChallengeCommandBuilder.parameterUSInt(static_cast<uint8_t>(userLevel)).build();
    CoLaCommand getChallengeResponse = m_VisionaryControl.sendCommand(getChallengeCommand);
    // check whether there occurred an error with the CoLa communication
    if (getChallengeResponse.getError() == CoLaError::OK)
    {
      // read and check response of GetChallenge command
      CoLaParameterReader coLaParameterReader = CoLaParameterReader(getChallengeResponse);
      isLoginSuccessful                       = loginImpl(userLevel, password, getChallengeResponse, SUL2);
    }
  }
  return isLoginSuccessful;
}

bool AuthenticationSecure::logout()
{
  CoLaCommand runCommand  = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "Run").build();
  CoLaCommand runResponse = m_VisionaryControl.sendCommand(runCommand);

  if (runResponse.getError() == CoLaError::OK)
  {
    return CoLaParameterReader(runResponse).readUSInt() != 0u;
  }
  return false;
}

} // namespace visionary
