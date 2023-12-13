// See LICENSE for license details

package aes

import chisel3._
import chisel3.util._

import org.chipsalliance.cde.config.{Parameters}
import freechips.rocketchip.util.{DecoupledHelper}
import testchipip.{StreamWidener, StreamNarrower}
import roccaccutils.logger._
import roccaccutils.memutils._
import roccaccutils.memstreamer._

import AES256Consts._

class AES256ECB(val logger: Logger = DefaultLogger)(implicit val p: Parameters, val hp: L2MemHelperParams) extends MemStreamer {
  class AES256ECBBundle extends MemStreamerBundle {
    val key = Flipped(Valid(UInt(AES256Consts.KEY_SZ_BITS.W))) //from CommandRouter
    val mode = Flipped(Valid(Bool())) //from CommandRouter
  }
  lazy val io = IO(new AES256ECBBundle)

  // Connect AES core to MemLoader (i.e. load_data_queue)

  val aes = Module(new AesCipherCoreDriver)

  assert(BLOCK_SZ_BITS <= BUS_SZ_BITS, "Need the bus bits to be greater than (or equal to) the block bits")

  val swidener = Module(new StreamWidener(BLOCK_SZ_BITS, BUS_SZ_BITS))

  val key = RegInit(0.U(AES256Consts.KEY_SZ_BITS.W))
  when (io.key.valid) {
    key := io.key.bits
  }
  val mode = RegInit(false.B)
  when (io.mode.valid) {
    mode := io.mode.bits
  }

  val last_queue = Module(new Queue(Bool(), 5)) // keep track of stream.bits.last in aes compute

  val na_fire = DecoupledHelper(
    io.mem_stream.req.ready,
    last_queue.io.enq.ready,
    aes.io.in.ready
  )

  val aw_fire = DecoupledHelper(
    aes.io.out.valid,
    last_queue.io.deq.valid,
    swidener.io.in.ready
  )

  last_queue.io.enq.bits := io.mem_stream.output_last_chunk
  last_queue.io.enq.valid := na_fire.fire(last_queue.io.enq.ready)
  last_queue.io.deq.ready := aw_fire.fire(last_queue.io.deq.valid)

  aes.io.in.bits.key := key
  aes.io.in.bits.encrypt := mode

  // Connect AES core input to MemLoader

  aes.io.in.bits.data := io.mem_stream.output_data
  aes.io.in.valid := na_fire.fire(aes.io.in.ready)
  io.mem_stream.req.valid := na_fire.fire(io.mem_stream.req.ready)
  io.mem_stream.req.bits := BLOCK_SZ_BYTES.U

  swidener.io.in.bits.data := aes.io.out.bits.data
  swidener.io.in.bits.keep := (1.U << BLOCK_SZ_BYTES) - 1.U
  swidener.io.in.bits.last := last_queue.io.deq.bits

  swidener.io.in.valid := aw_fire.fire(swidener.io.in.ready)
  aes.io.out.ready := aw_fire.fire(aes.io.out.valid)

  // Connect AES core output to MemWriter

  store_data_queue.io.enq.bits.chunk_data := swidener.io.out.bits.data
  store_data_queue.io.enq.bits.chunk_size_bytes := PopCount(swidener.io.out.bits.keep)
  store_data_queue.io.enq.bits.is_final_chunk := swidener.io.out.bits.last

  val write_fire = DecoupledHelper(
    swidener.io.out.valid,
    store_data_queue.io.enq.ready
  )
  store_data_queue.io.enq.valid := write_fire.fire(store_data_queue.io.enq.ready)
  swidener.io.out.ready := write_fire.fire(swidener.io.out.valid)
}
