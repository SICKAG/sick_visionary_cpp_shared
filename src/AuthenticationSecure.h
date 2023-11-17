//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once
#include <array>

#include "CoLaParameterReader.h"
#include "VisionaryControl.h"

namespace visionary {
enum ProtocolType
{
  UNKNOWN,
  SUL1,
  SUL2
};
struct ChallengeRequest
{
  std::array<std::uint8_t, 16> challenge;
  std::array<std::uint8_t, 16> salt;
};

typedef std::array<std::uint8_t, 32> PasswordHash;
typedef std::array<std::uint8_t, 32> ChallengeResponse;

class AuthenticationSecure : public IAuthentication
{
public:
  explicit AuthenticationSecure(VisionaryControl& vctrl);
  ~AuthenticationSecure() override;

  bool login(UserLevel userLevel, const std::string& password) override;
  bool logout() override;

private:
  VisionaryControl& m_VisionaryControl;
  ProtocolType      m_protocolType;

  PasswordHash      CreatePasswordHash(UserLevel               userLevel,
                                       const std::string&      password,
                                       const ChallengeRequest& challengeRequest,
                                       ProtocolType            protocolType);
  ChallengeResponse CreateChallengeResponse(UserLevel               userLevel,
                                            const std::string&      password,
                                            const ChallengeRequest& challengeRequest,
                                            ProtocolType            protocolType);
  bool              loginImpl(UserLevel                  userLevel,
                              const std::string&         password,
                              const CoLaParameterReader& getChallengeResponse,
                              ProtocolType               protocolType);
};

} // namespace visionary
