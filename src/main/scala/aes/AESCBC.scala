// See LICENSE for license details

package aes

import chisel3._
import chisel3.util._

import org.chipsalliance.cde.config.{Parameters}
import freechips.rocketchip.util.{DecoupledHelper}
import testchipip.serdes.{StreamWidener, StreamNarrower}
import roccaccutils._
import roccaccutils.logger._

import AESConsts._

class AESCBC(keySzBits: Int, val logger: Logger = DefaultLogger)(implicit val p: Parameters, val hp: L2MemHelperParams) extends MemStreamer {
  class AESCBCBundle extends MemStreamerBundle {
    val key = Flipped(Valid(UInt(keySzBits.W))) //from CommandRouter
    val mode = Flipped(Valid(Bool())) //from CommandRouter
    val iv = Flipped(Valid(UInt(CBCConsts.IV_SZ_BITS.W))) //from CommandRouter
  }
  lazy val io = IO(new AESCBCBundle)

  // Connect AES core to MemLoader (i.e. load_data_queue)

  val aes = Module(new AESCipherCoreDriver(keySzBits))

  assert(BLOCK_SZ_BITS <= BUS_SZ_BITS, "Need the bus bits to be greater than (or equal to) the block bits")

  val snarrower = Module(new StreamNarrower(BUS_SZ_BITS, BLOCK_SZ_BITS))
  val swidener = Module(new StreamWidener(BLOCK_SZ_BITS, BUS_SZ_BITS))

  val key = RegInit(0.U(keySzBits.W))
  when (io.key.valid) {
    key := io.key.bits
  }
  val mode = RegInit(false.B)
  when (io.mode.valid) {
    mode := io.mode.bits
  }

  val last_queue = Module(new Queue(Bool(), 5)) // keep track of stream.bits.last in aes compute

  // snarrower -> aes
  val na_fire = DecoupledHelper(
    snarrower.io.out.valid,
    last_queue.io.enq.ready,
    aes.io.in.ready
  )

  // aes -> swidener
  val aw_fire = DecoupledHelper(
    aes.io.out.valid,
    last_queue.io.deq.valid,
    swidener.io.in.ready
  )

  val xor_queue = Module(new Queue(UInt(BLOCK_SZ_BITS.W), 5, hasFlush=true))
  xor_queue.io.enq.valid := false.B
  xor_queue.io.enq.bits := DontCare
  when (io.iv.valid) {
    xor_queue.io.enq.valid := true.B
    xor_queue.io.enq.bits := io.iv.bits
  } .elsewhen(mode && aes.io.out.fire) {
    xor_queue.io.enq.valid := true.B
    xor_queue.io.enq.bits := aes.io.out.bits.data
  } .elsewhen(!mode && aes.io.in.fire) {
    xor_queue.io.enq.valid := true.B
    xor_queue.io.enq.bits := aes.io.in.bits.data
  }

  // TODO:
  //  for encryption - delay aes.io.in.valid until aes.io.out valid was triggered (s.t. you get a new xor val) after the 1st
  //  for decryption - no delay, just store previous ciphertext value and use as xor

  val enc_xor_valid = !mode || (mode && xor_queue.io.deq.valid)
  val dec_xor_valid = mode || (!mode && xor_queue.io.deq.valid)

  last_queue.io.enq.bits := snarrower.io.out.bits.last

  last_queue.io.enq.valid := na_fire.fire(last_queue.io.enq.ready) && enc_xor_valid
  last_queue.io.deq.ready := aw_fire.fire(last_queue.io.deq.valid) && dec_xor_valid

  aes.io.in.bits.key := key
  aes.io.in.bits.encrypt := mode

  aes.io.in.bits.data := Mux(mode, snarrower.io.out.bits.data ^ xor_queue.io.deq.bits, snarrower.io.out.bits.data)
  aes.io.in.valid := na_fire.fire(aes.io.in.ready) && enc_xor_valid
  snarrower.io.out.ready := na_fire.fire(snarrower.io.out.valid) && enc_xor_valid

  swidener.io.in.bits.data := Mux(!mode, aes.io.out.bits.data ^ xor_queue.io.deq.bits, aes.io.out.bits.data)
  swidener.io.in.bits.keep := (1.U << BLOCK_SZ_BYTES) - 1.U
  swidener.io.in.bits.last := last_queue.io.deq.bits
  swidener.io.in.valid := aw_fire.fire(swidener.io.in.ready) && dec_xor_valid
  aes.io.out.ready := aw_fire.fire(aes.io.out.valid) && dec_xor_valid

  xor_queue.io.deq.ready := Mux(mode, na_fire.fire(), aw_fire.fire())

  snarrower.io.in.bits.data := load_data_queue.io.deq.bits.chunk_data
  snarrower.io.in.bits.keep := (1.U << load_data_queue.io.deq.bits.chunk_size_bytes) - 1.U
  snarrower.io.in.bits.last := load_data_queue.io.deq.bits.is_final_chunk

  val narrow_fire = DecoupledHelper(
    load_data_queue.io.deq.valid,
    snarrower.io.in.ready,
  )
  snarrower.io.in.valid := narrow_fire.fire(snarrower.io.in.ready)
  load_data_queue.io.deq.ready := narrow_fire.fire(load_data_queue.io.deq.valid)

  // Connect AES core output to MemWriter (i.e. store_data_queue)

  store_data_queue.io.enq.bits.chunk_data := swidener.io.out.bits.data
  store_data_queue.io.enq.bits.chunk_size_bytes := PopCount(swidener.io.out.bits.keep)
  store_data_queue.io.enq.bits.is_final_chunk := swidener.io.out.bits.last

  val write_fire = DecoupledHelper(
    swidener.io.out.valid,
    store_data_queue.io.enq.ready
  )
  store_data_queue.io.enq.valid := write_fire.fire(store_data_queue.io.enq.ready)
  swidener.io.out.ready := write_fire.fire(swidener.io.out.valid)

  xor_queue.io.flush.map( _ := write_fire.fire() && swidener.io.out.bits.last )
}
