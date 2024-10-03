// See LICENSE for license details

package aes

import chisel3._
import chisel3.util._

import org.chipsalliance.cde.config.{Config, Parameters, Field}
import freechips.rocketchip.tile.{BuildRoCC, OpcodeSet}
import freechips.rocketchip.rocket.{TLBConfig}
import freechips.rocketchip.diplomacy.{LazyModule}
import roccaccutils._

case object AESCBCAccelInsertXbarBetweenMemory extends Field[Boolean](true)
case object AESCBCAccelCmdQueueDepth extends Field[Int](2)

class WithAESCBCAccel extends Config ((site, here, up) => {
  case AESCBCAccelTLB => Some(TLBConfig(nSets = 4, nWays = 4, nSectors = 1, nSuperpageEntries = 1))
  case BuildRoCC => up(BuildRoCC) ++ Seq(
    (p: Parameters) => {
      val acc = LazyModule(new AESCBCAccel(OpcodeSet.custom1)(p))
      acc
    }
  )
})
