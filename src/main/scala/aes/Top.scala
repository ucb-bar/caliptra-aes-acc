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
import roccaccutils._

case object AESCBCAccelTLB extends Field[Option[TLBConfig]](None)

class AESCBCAccel(opcodes: OpcodeSet, val keySzBits: Int = 256)(implicit p: Parameters) extends MemStreamerAccel(
  opcodes = opcodes) {

  override lazy val module = new AESCBCAccelImp(this)

  require(p(SystemBusKey).beatBytes == 32, "Only tested on 32B SBUS width") // TODO: should work for 128b bus

  lazy val tlbConfig = p(AESCBCAccelTLB).get
  lazy val xbarBetweenMem = p(AESCBCAccelInsertXbarBetweenMemory)
  lazy val logger = AESCBCLogger
}

class AESCBCAccelImp(outer: AESCBCAccel)(implicit p: Parameters)
  extends MemStreamerAccelImp(outer) {

  lazy val queueDepth = p(AESCBCAccelCmdQueueDepth)

  lazy val cmd_router = Module(new CommandRouter(outer.keySzBits, queueDepth))
  lazy val streamer = Module(new AESCBC(outer.keySzBits, outer.logger))

  streamer.io.key <> cmd_router.io.key
  streamer.io.mode <> cmd_router.io.mode
  streamer.io.iv <> cmd_router.io.iv
}
