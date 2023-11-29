// See LICENSE for license details

package aes

import chisel3._

import org.chipsalliance.cde.config.{Parameters, Field}
import freechips.rocketchip.tile._
import freechips.rocketchip.rocket.{TLBConfig}
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.subsystem.{SystemBusKey}
import freechips.rocketchip.rocket.constants.MemoryOpConstants
import freechips.rocketchip.tilelink._
import roccaccutils.memutils._
import roccaccutils.memstreamer._

case object AES256AccelTLB extends Field[Option[TLBConfig]](None)

class AES256ECBAccel(opcodes: OpcodeSet)(implicit p: Parameters) extends MemStreamerRoCC(
  opcodes = opcodes) {

  override lazy val module = new AES256ECBAccelImp(this)

  require(p(SystemBusKey).beatBytes == 32, "Only tested on 32B SBUS width") // TODO: should work for 128b

  lazy val tlbConfig = p(AES256AccelTLB).get
  lazy val xbarBetweenMem = p(AES256ECBAccelInsertXbarBetweenMemory)
  lazy val logger = AES256ECBLogger
}

class AES256ECBAccelImp(outer: AES256ECBAccel)(implicit p: Parameters)
  extends MemStreamerRoCCImp(outer) {

  lazy val metadataQueueDepth = p(AES256ECBAccelCmdQueueDepth)
  lazy val dataQueueDepth = p(AES256ECBAccelDataQueueDepth)

  lazy val cmd_router = Module(new CommandRouter(metadataQueueDepth))
  lazy val streamer = Module(new AES256ECB(outer.logger))

  streamer.io.key <> cmd_router.io.key
  streamer.io.mode <> cmd_router.io.mode
}
