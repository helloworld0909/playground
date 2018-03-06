package com.helloworld09.playground.scala.element

import org.junit.Assert._
import org.junit.Test

class ElementTest {

  @Test
  def testAbove(): Unit = {
    val elem1 = Element.elem(Array("test", "case"))
    val elem2 = Element.elem(Array("casecase", "testtest"))
    assertTrue((elem1 above elem2).contents sameElements Array("  test  ", "  case  ", "casecase", "testtest"))
  }

  @Test
  def testBeside(): Unit = {
    val elem1 = Element.elem(Array("test", "case", "pass"))
    val elem2 = Element.elem(Array("case", "test"))
    assertTrue((elem1 beside elem2).contents sameElements Array("testcase", "casetest", "pass    "))
  }

  @Test
  def testUniformElement(): Unit = {
    println(Element.elem('*', 0, 5))
  }

  @Test
  def testStringUniformElement(): Unit = {
    val elem1 = Element.elem("test", 2, 6)
    assertTrue(elem1.contents sameElements Array("testte", "testte"))
  }

  @Test
  def testWiden(): Unit = {
    val elem1 = Element.elem("test", 2, 6)
    assertTrue(elem1.widen(7).contents sameElements Element.elem(Array("testte ", "testte ")).contents)
  }

  @Test
  def testHeighten(): Unit = {
    val elem1 = Element.elem('*', 2, 6)
    assertTrue(elem1.heighten(3).contents sameElements Element.elem(Array("*" * 6, "*" * 6, " " * 6)).contents)
  }
}
