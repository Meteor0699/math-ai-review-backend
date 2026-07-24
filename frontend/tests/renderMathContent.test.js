import assert from 'node:assert/strict'
import test from 'node:test'

import { renderMathContent } from '../src/utils/renderMathContent.js'

test('renders trusted math while escaping database HTML', () => {
  const html = renderMathContent('<img src=x onerror="alert(1)"> 当 $x^2$ 增大')

  assert.match(html, /katex/)
  assert.doesNotMatch(html, /<img/i)
  assert.match(html, /&lt;img/)
  assert.match(html, /onerror=&quot;/)
})

test('does not allow javascript links or script elements', () => {
  const html = renderMathContent('<script>alert(1)</script><a href="javascript:alert(1)">点击</a>')

  assert.doesNotMatch(html, /<script/i)
  assert.doesNotMatch(html, /<a/i)
  assert.match(html, /&lt;script/)
})
