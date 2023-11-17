//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include "FrameGrabberBase.h"
#include "VisionaryDataStream.h"
#include <mutex>
#include <thread>

namespace visionary {
/**
 * \brief Class which receives frames from the device in background thread and provides the latest one via an interface.
 * This helps avoiding getting old frames because of buffering of data within the network infrastructure.
 * It also handles automatically reconnects in case of connection issues.
 */
template <class DataType>
class FrameGrabber
{
public:
  FrameGrabber(const std::string& hostname, std::uint16_t port, std::uint32_t timeoutMs)
    : frameGrabberBase(hostname, port, timeoutMs)
  {
    frameGrabberBase.start(std::make_shared<DataType>(), std::make_shared<DataType>());
  }
  ~FrameGrabber()
  {
  }

  /// Gets the next blob from the connected device
  /// \param[in, out] pDataHandler an (empty) pointer where the blob will be stored in
  /// \param[in] timeoutMs controls the timeout how long to wait for a new blob, default 1000ms
  ///
  /// \retval true New blob has been received and stored in pDataHandler Pointer
  /// \retval false No new blob has been received
  bool getNextFrame(std::shared_ptr<DataType>& pDataHandler, std::uint32_t timeoutMs = 1000)
  {
    if (pDataHandler == nullptr)
      pDataHandler = std::make_shared<DataType>();
    auto       pTypedDataHandler = std::move(std::dynamic_pointer_cast<VisionaryData>(pDataHandler));
    const auto retVal            = frameGrabberBase.getNextFrame(pTypedDataHandler, timeoutMs);
    pDataHandler                 = std::move(std::dynamic_pointer_cast<DataType>(pTypedDataHandler));
    return retVal;
  }

  /// Gets the current blob from the connected device
  /// \param[in, out] pDataHandler an (empty) pointer where the blob will be stored in
  ///
  /// \retval true a blob was available and has been stored in pDataHandler Pointer
  /// \retval false No blob was available
  bool getCurrentFrame(std::shared_ptr<DataType>& pDataHandler)
  {
    if (pDataHandler == nullptr)
      pDataHandler = std::make_shared<DataType>();
    auto       pTypedDataHandler = std::move(std::dynamic_pointer_cast<VisionaryData>(pDataHandler));
    const auto retVal            = frameGrabberBase.getCurrentFrame(pTypedDataHandler);
    pDataHandler                 = std::move(std::dynamic_pointer_cast<DataType>(pTypedDataHandler));
    return retVal;
  }

private:
  FrameGrabberBase frameGrabberBase;
};
} // namespace visionary
