// Copyright 2004-present Facebook. All Rights Reserved.

#include "rsocket/statemachine/StreamStateMachineBase.h"

#include <folly/io/IOBuf.h>

#include "rsocket/statemachine/RSocketStateMachine.h"
#include "rsocket/statemachine/StreamsWriter.h"

namespace rsocket {

void StreamStateMachineBase::handlePayload(Payload&&, bool, bool) {
  VLOG(4) << "Unexpected handlePayload";
}

void StreamStateMachineBase::handleRequestN(uint32_t) {
  VLOG(4) << "Unexpected handleRequestN";
}

void StreamStateMachineBase::handleError(folly::exception_wrapper) {
  endStream(StreamCompletionSignal::ERROR);
  removeFromWriter();
}

void StreamStateMachineBase::handleCancel() {
  VLOG(4) << "Unexpected handleCancel";
}

size_t StreamStateMachineBase::getConsumerAllowance() const {
  return 0;
}

void StreamStateMachineBase::newStream(
    StreamType streamType,
    uint32_t initialRequestN,
    Payload payload) {
  DCHECK_NE(streamId_, 0);
  writer_.writeNewStream(
      streamId_, streamType, initialRequestN, std::move(payload));
}

void StreamStateMachineBase::writeRequestN(uint32_t n) {
  DCHECK_NE(streamId_, 0);
  writer_.writeRequestN(Frame_REQUEST_N{streamId_, n});
}

void StreamStateMachineBase::writeCancel() {
  DCHECK_NE(streamId_, 0);
  writer_.writeCancel(Frame_CANCEL{streamId_});
}

void StreamStateMachineBase::writePayload(Payload&& payload, bool complete) {
  DCHECK_NE(streamId_, 0);
  auto const flags =
      FrameFlags::NEXT | (complete ? FrameFlags::COMPLETE : FrameFlags::EMPTY);
  Frame_PAYLOAD frame{streamId_, flags, std::move(payload)};
  writer_.writePayload(std::move(frame));
}

void StreamStateMachineBase::writeComplete() {
  DCHECK_NE(streamId_, 0);
  writer_.writePayload(Frame_PAYLOAD::complete(streamId_));
}

void StreamStateMachineBase::writeApplicationError(folly::StringPiece msg) {
  DCHECK_NE(streamId_, 0);
  writer_.writeError(Frame_ERROR::applicationError(streamId_, msg));
}

void StreamStateMachineBase::writeInvalidError(folly::StringPiece msg) {
  DCHECK_NE(streamId_, 0);
  writer_.writeError(Frame_ERROR::invalid(streamId_, msg));
}

void StreamStateMachineBase::removeFromWriter() {
  // TODO: Ideally we'd do `DCHECK_NE(streamId_, 0)` here too, but that causes
  // crashes with channel tests.

  if (streamId_ != 0) {
    writer_.onStreamClosed(streamId_);
    streamId_ = 0;
  }
}

} // namespace rsocket
