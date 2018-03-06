package com.helloworld09.playground.scala.element.example

import com.helloworld09.playground.scala.element.{Element, AbstractElement}

class Spiral {

  val corner: AbstractElement = Element.elem("+")
  val space: AbstractElement = Element.elem(" ")

  def nextDirection(direction: Int): Int = (direction + 3) % 4

  def draw(direction: Int, size: Int): AbstractElement = {
    if (size == 1) {
      corner
    }
    else {
      val rest = draw(nextDirection(direction), size - 1)
      direction match {
        case 0 =>
          val horizontalLine = corner beside Element.elem('—', 1, rest.width)
          horizontalLine above (rest beside space)
        case 1 =>
          val verticalLine = corner above Element.elem('|', rest.height, 1)
          (rest above space) beside verticalLine
        case 2 =>
          val horizontalLine = Element.elem('—', 1, rest.width) beside corner
          (space beside rest) above horizontalLine
        case 3 =>
          val verticalLine = Element.elem('|', rest.height, 1) above corner
          verticalLine beside (space above rest)
      }
    }
  }
}

object Spiral {
  def main(args: Array[String]): Unit = {
    val s = (new Spiral).draw(0, 100)
    println(s)
  }
}