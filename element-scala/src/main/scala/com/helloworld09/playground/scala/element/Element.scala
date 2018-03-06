package com.helloworld09.playground.scala.element


abstract class AbstractElement {

  val contents: Array[String]

  def height: Int = contents.length

  def width: Int = height match {
    case 0 => 0
    case _ => contents.map(_.length).max
  }

  override def toString: String = contents.mkString("\n")

  def above(that: AbstractElement): AbstractElement = {
    Element.elem(this.widen(that.width).contents ++ that.widen(this.width).contents)
  }

  def beside(that: AbstractElement): AbstractElement = {
    val newContents =
      for (
        twoLines <- this.heighten(that.height).contents zip that.heighten(this.height).contents
      ) yield twoLines._1 + twoLines._2
    Element.elem(newContents)
  }

  def widen(w: Int): AbstractElement = {
    if (width >= w) {
      this
    }
    else {
      val leftWidth = (w - width) / 2
      val rightWidth = w - width - leftWidth
      val left = Element.elem(' ', height, leftWidth)
      val right = Element.elem(' ', height, rightWidth)
      left beside this beside right
    }
  }

  def heighten(h: Int): AbstractElement = {
    if (height >= h) {
      this
    }
    else {
      val topHeight = (h - height) / 2
      val bottomHeight = h - height - topHeight
      val top = Element.elem(' ', topHeight, width)
      val bottom = Element.elem(' ', bottomHeight, width)
      top above this above bottom
    }
  }
}


object Element {

  private class ArrayElement(val contents: Array[String]) extends AbstractElement

  private class LineElement(s: String) extends ArrayElement(Array(s)) {
    override val height: Int = 1
    override val width: Int = s.length
  }

  private class UniformElement(val char: Char, override val height: Int, override val width: Int) extends AbstractElement {
    private val line = char.toString * width

    override val contents: Array[String] = Array.fill(height)(line)
  }

  def elem(contents: Array[String]): AbstractElement = {
    new ArrayElement(contents)
  }

  def elem(s: String): AbstractElement = {
    new LineElement(s)
  }

  def elem(char: Char, height: Int, width: Int): AbstractElement = {
    new UniformElement(char, height, width)
  }

  def elem(s: String, height: Int, width: Int): AbstractElement = {
    val repeat: Int = width / s.length
    val residual = width - repeat * s.length
    val line = s * repeat + s.substring(0, residual)
    new ArrayElement(Array.fill(height)(line))
  }
}




