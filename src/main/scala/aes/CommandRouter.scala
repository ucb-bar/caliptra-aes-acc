// See LICENSE for license details

package aes

import chisel3._
import chisel3.util._

import org.chipsalliance.cde.config.{Parameters}
import freechips.rocketchip.util.DecoupledHelper
import roccaccutils.memutils._
import roccaccutils.memstreamer._

class CommandRouter(val cmdQueueDepth: Int)(implicit val p: Parameters) extends MemStreamerCommandRouter {
  class AesStreamerCmdBundle()(implicit p: Parameters) extends MemStreamerCmdBundle {
    val key = Valid(UInt(AES256Consts.KEY_SZ_BITS.W))
    val mode = Valid(Bool())
  }
  lazy val io = IO(new AesStreamerCmdBundle) // lazy matters

  val FUNCT_MODE                          = 4.U
  val FUNCT_KEY_0                         = 5.U
  val FUNCT_KEY_1                         = 6.U

  // Mode interface
  val mode_queue = Module(new Queue(Bool(), cmdQueueDepth))
  mode_queue.io.enq.bits := cur_rs1
  val mode_fire = DecoupledHelper(
    io.rocc_in.valid,
    cur_funct === FUNCT_MODE,
    mode_queue.io.enq.ready
  )
  mode_queue.io.enq.valid := mode_fire.fire(mode_queue.io.enq.ready)
  io.mode.bits <> mode_queue.io.deq.bits
  io.mode.valid <> mode_queue.io.deq.valid
  mode_queue.io.deq.ready := true.B

  // Key interface
  val key_queue = Module(new Queue(UInt(AES256Consts.KEY_SZ_BITS.W), cmdQueueDepth))
  val key_lower_128 = RegInit(0.U(128.W))
  val key0_fire = DecoupledHelper(
    io.rocc_in.valid,
    cur_funct === FUNCT_KEY_0,
    key_queue.io.enq.ready
  )
  when (key0_fire.fire(key_queue.io.enq.ready)) {
    key_lower_128 := Cat(cur_rs1, cur_rs2)
  }
  val key1_fire = DecoupledHelper(
    io.rocc_in.valid,
    cur_funct === FUNCT_KEY_1,
    key_queue.io.enq.ready
  )
  key_queue.io.enq.valid := key1_fire.fire(key_queue.io.enq.ready)
  key_queue.io.enq.bits := Cat(Cat(cur_rs1, cur_rs2), key_lower_128)
  io.key.bits <> key_queue.io.deq.bits
  io.key.valid <> key_queue.io.deq.valid
  key_queue.io.deq.ready := true.B

  // streaming_fire provided by MemStreamerCommandRouter
  io.rocc_in.ready := streaming_fire ||
    mode_fire.fire(io.rocc_in.valid) ||
    key0_fire.fire(io.rocc_in.valid) ||
    key1_fire.fire(io.rocc_in.valid)
}
