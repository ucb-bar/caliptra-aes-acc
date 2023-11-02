// See LICENSE for license details

package aes

import chisel3._
import chisel3.util._

import org.chipsalliance.cde.config.{Parameters, Field}
import midas.targetutils.{SynthesizePrintf}
import roccaccutils._

case object AES256ECBAccelPrintfSynth extends Field[Boolean](true)

object AES256ECBAccelLogger extends AccelLogger {
  // just print info msgs
  def logInfoImplPrintWrapper(printf: chisel3.printf.Printf)(implicit p: Parameters): chisel3.printf.Printf = {
    printf
  }

  // optionally synthesize critical msgs
  def logCriticalImplPrintWrapper(printf: chisel3.printf.Printf)(implicit p: Parameters): chisel3.printf.Printf = {
    if (p(AES256ECBAccelPrintfSynth)) {
      SynthesizePrintf(printf)
    } else {
      printf
    }
  }
}
