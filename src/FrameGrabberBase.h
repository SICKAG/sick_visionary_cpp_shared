//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include "VisionaryDataStream.h"
#include <condition_variable>
#include <mutex>
#include <thread>

namespace visionary {
class FrameGrabberBase
{
public:
  FrameGrabberBase(const std::string& hostname, std::uint16_t port, std::uint32_t timeoutMs);
  ~FrameGrabberBase();

  void start(std::shared_ptr<VisionaryData> inactiveDataHandler, std::shared_ptr<VisionaryData> activeDataHandler);
  bool getNextFrame(std::shared_ptr<VisionaryData>& pDataHandler, std::uint32_t timeoutMs = 1000);
  bool getCurrentFrame(std::shared_ptr<VisionaryData>& pDataHandler);

private:
  void                                 run();
  bool                                 m_isRunning;
  bool                                 m_FrameAvailable;
  bool                                 m_connected;
  const std::string                    m_hostname;
  const std::uint16_t                  m_port;
  const std::uint32_t                  m_timeoutMs;
  std::unique_ptr<VisionaryDataStream> m_pDataStream;
  std::thread                          m_grabberThread;
  std::shared_ptr<VisionaryData>       m_pDataHandler;
  std::mutex                           m_dataHandler_mutex;
  std::condition_variable              m_frameAvailableCv;
};
} // namespace visionary
