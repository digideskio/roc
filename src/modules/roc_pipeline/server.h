/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/server.h
//! @brief Server pipeline.

#ifndef ROC_PIPELINE_SERVER_H_
#define ROC_PIPELINE_SERVER_H_

#include "roc_core/noncopyable.h"
#include "roc_core/maybe.h"
#include "roc_core/thread.h"
#include "roc_core/atomic.h"

#include "roc_datagram/idatagram.h"
#include "roc_datagram/idatagram_reader.h"

#include "roc_packet/ipacket_parser.h"

#include "roc_audio/isample_buffer_writer.h"
#include "roc_audio/isink.h"
#include "roc_audio/channel_muxer.h"
#include "roc_audio/timed_writer.h"
#include "roc_audio/delayed_writer.h"

#include "roc_pipeline/session_manager.h"
#include "roc_pipeline/config.h"

namespace roc {
namespace pipeline {

//! Server pipeline.
//!
//! Fetches datagrams from input queue, manages active sessions and their
//! storages and renderers, and generates audio stream.
//!
//! @b Queues
//!  - Input datagram queue is usually passed to network thread which writes
//!    incoming datagrams to it.
//!
//!  - Output sample buffer queue is usually passed to audio player thread
//!    which fetches samples from it and sends them to the sound card.
//!
//! @b Invocation
//!  - User may call start() to start server thread. The thread will call
//!    tick() in an infinite loop.
//!
//!  - Alternatively, user may periodically call tick().
//!
//! @b Pipeline
//!
//!  Server pipeline consists of several steps:
//!
//!   <i> Fetching datagrams </i>
//!    - Fetch datagrams from input queue.
//!
//!    - Look at datagram's source address and check if a session exists for
//!      this address; if not, and parser exists for datagram's destination
//!      address, create new session using session pool.
//!
//!    - If new session was created, attach it to audio sink.
//!
//!    - If session existed or created, parse packet from datagram and store
//!      new packet into session.
//!
//!   <i> Updating state </i>
//!    - Update every session state.
//!
//!    - If session fails to update its state (probably bacause it detected
//!      that it's broken or inactive), session is unregistered from
//!      audio sink and removed.
//!
//!   <i> Generating samples </i>
//!    - Requests audio sink to generate samples. During this process,
//!      previously stored packets are transformed into audio stream.
//!
//! @see ServerConfig, Session
class Server : public core::Thread, public core::NonCopyable<> {
public:
    //! Initialize server.
    //!
    //! @b Parameters
    //!  - @p datagram_reader specifies input datagram queue;
    //!  - @p audio_writer specifies output sample queue;
    //!  - @p config specifies server and session configuration.
    Server(datagram::IDatagramReader& datagram_reader,
           audio::ISampleBufferWriter& audio_writer,
           const ServerConfig& config = ServerConfig());

    //! Get number of active sessions.
    size_t num_sessions() const;

    //! Register port.
    //! @remarks
    //!  When datagram received with destination @p address, session will
    //!  use @p parser to create packet from datagram. If no port registered
    //!  for address, datagrams to that address will be dropped.
    void add_port(const datagram::Address& address, packet::IPacketParser& parser);

    //! Process input datagrams.
    //! @remarks
    //!  Fetches datagrams from input datagram reader and generates next
    //!  sample buffer.
    bool tick();

    //! Stop thread.
    //! @remarks
    //!  May be called from any thread. After this call, subsequent join()
    //!  call will return as soon as current tick() returns.
    void stop();

private:
    virtual void run();

    const ServerConfig config_;
    const size_t n_channels_;

    core::Atomic stop_;

    audio::ChannelMuxer channel_muxer_;
    core::Maybe<audio::TimedWriter> timed_writer_;
    audio::DelayedWriter delayed_writer_;

    datagram::IDatagramReader& datagram_reader_;
    audio::ISampleBufferWriter* audio_writer_;

    SessionManager session_manager_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SERVER_H_
