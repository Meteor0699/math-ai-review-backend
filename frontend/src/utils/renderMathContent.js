import katex from 'katex'

function escapeHtml(value) {
  return String(value ?? '')
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;')
}

function renderFormula(formula, displayMode) {
  const normalizedFormula = formula.replace(/\\\\/g, '\\').trim()
  return katex.renderToString(normalizedFormula, {
    displayMode,
    throwOnError: false,
    strict: false,
    trust: false
  })
}

function stashMath(text, tokens, pattern, displayMode) {
  return text.replace(pattern, (_, formula) => {
    const key = `@@MATH_${tokens.length}@@`
    tokens.push(renderFormula(formula, displayMode))
    return key
  })
}

function stashInlineDollarMath(text, tokens) {
  return text.replace(/(^|[^\\$])\$([^$\n]+?)\$/g, (_, prefix, formula) => {
    const key = `@@MATH_${tokens.length}@@`
    tokens.push(renderFormula(formula, false))
    return `${prefix}${key}`
  })
}

function renderBasicMarkdown(text) {
  let html = escapeHtml(text)

  html = html.replace(/^###\s+(.+)$/gm, '<h5>$1</h5>')
  html = html.replace(/^##\s+(.+)$/gm, '<h4>$1</h4>')
  html = html.replace(/^#\s+(.+)$/gm, '<h3>$1</h3>')
  html = html.replace(/\*\*(.+?)\*\*/g, '<strong>$1</strong>')
  html = html.replace(/`([^`]+)`/g, '<code>$1</code>')

  return html
    .split(/\n{2,}/)
    .map((paragraph) => paragraph.trim())
    .filter(Boolean)
    .map((paragraph) => `<p>${paragraph.replace(/\n/g, '<br>')}</p>`)
    .join('')
}

export function renderMathContent(value) {
  if (!value) return ''

  const tokens = []
  let text = String(value).replace(/\r\n/g, '\n')
  text = text
    .replace(/\\\\\(/g, '\\(')
    .replace(/\\\\\)/g, '\\)')
    .replace(/\\\\\[/g, '\\[')
    .replace(/\\\\\]/g, '\\]')

  text = stashMath(text, tokens, /\\\[([\s\S]+?)\\\]/g, true)
  text = stashMath(text, tokens, /\$\$([\s\S]+?)\$\$/g, true)
  text = stashMath(text, tokens, /\\\(([\s\S]+?)\\\)/g, false)
  text = stashInlineDollarMath(text, tokens)

  let html = renderBasicMarkdown(text)
  tokens.forEach((formulaHtml, index) => {
    html = html.replace(`@@MATH_${index}@@`, formulaHtml)
  })
  return html
}
