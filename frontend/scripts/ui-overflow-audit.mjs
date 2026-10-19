// UI overflow audit: logs in, walks every route (desktop + mobile, student +
// admin) and reports real layout overflow — text clipped without an ellipsis
// or tooltip, content wider than its box, elements spilling past their parent
// or past the viewport — plus a full page screenshot per route.
//
// Usage (from frontend/):
//   E2E_STUDENT_USERNAME=student E2E_STUDENT_PASSWORD=... \
//   E2E_ADMIN_USERNAME=admin E2E_ADMIN_PASSWORD=... \
//   node scripts/ui-overflow-audit.mjs
import { chromium } from '@playwright/test'
import { mkdir, writeFile } from 'node:fs/promises'
import path from 'node:path'

const baseURL = process.env.E2E_BASE_URL || 'http://127.0.0.1:5173'
const outputDirectory = process.env.UI_AUDIT_DIR || path.resolve('ui-audit')
const credentials = {
  student: {
    username: process.env.E2E_STUDENT_USERNAME,
    password: process.env.E2E_STUDENT_PASSWORD
  },
  admin: {
    username: process.env.E2E_ADMIN_USERNAME,
    password: process.env.E2E_ADMIN_PASSWORD
  }
}

if (!credentials.student.username || !credentials.admin.username) {
  throw new Error('E2E student and admin credentials are required')
}

const studentRoutes = [
  ['student-home', '/home'],
  ['student-courses', '/courses'],
  ['student-course-detail', '/courses/1'],
  ['student-chapter', '/chapters/1'],
  ['student-knowledge', '/knowledge/1'],
  ['student-questions', '/questions'],
  ['student-question-detail', '/questions/1'],
  ['student-study', '/study'],
  ['student-papers', '/papers']
]

const adminRoutes = [
  ['admin-home', '/admin/home'],
  ['admin-courses', '/admin/courses'],
  ['admin-chapters', '/admin/chapters'],
  ['admin-knowledge', '/admin/knowledge'],
  ['admin-questions', '/admin/questions'],
  ['admin-papers', '/admin/papers'],
  ['admin-users', '/admin/users']
]

const auditPage = () => {
  const viewportWidth = window.innerWidth
  const findings = []
  const seen = new Set()

  const describe = (element) => {
    const classes = typeof element.className === 'string'
      ? element.className.split(/\s+/).filter(Boolean).slice(0, 3).join('.')
      : ''
    return element.tagName.toLowerCase() + (element.id ? '#' + element.id : '') + (classes ? '.' + classes : '')
  }
  const snippet = (element) => (element.textContent || '').replace(/\s+/g, ' ').trim().slice(0, 60)
  const scrollable = (element, axis) => {
    const style = getComputedStyle(element)
    const value = axis === 'x' ? style.overflowX : style.overflowY
    return value === 'auto' || value === 'scroll'
  }
  const insideScroller = (element, axis) => {
    let node = element.parentElement
    while (node && node !== document.body) {
      if (scrollable(node, axis)) return true
      node = node.parentElement
    }
    return false
  }
  // Clipping a user is expected to accept: CSS ellipsis, a line clamp, or an
  // Element Plus overflow tooltip that reveals the full text on hover.
  const intentionalClip = (element, style) =>
    style.textOverflow === 'ellipsis' ||
    element.hasAttribute('title') ||
    element.classList.contains('el-tooltip') ||
    (style.webkitLineClamp && style.webkitLineClamp !== 'none')

  const contentRight = (element) => {
    const box = element.getBoundingClientRect()
    const style = getComputedStyle(element)
    return box.right - (parseFloat(style.paddingRight) || 0) - (parseFloat(style.borderRightWidth) || 0)
  }

  for (const element of document.querySelectorAll('body *')) {
    const rect = element.getBoundingClientRect()
    if (rect.width < 1 || rect.height < 1) continue
    const style = getComputedStyle(element)
    if (style.visibility === 'hidden' || style.display === 'none' || style.display === 'inline') continue
    if (element.tagName === 'SVG' || element.ownerSVGElement) continue

    const selector = describe(element)
    const parent = element.parentElement
    const record = (kind, extra) => {
      const key = kind + '|' + selector
      if (seen.has(key)) return
      seen.add(key)
      findings.push({
        kind,
        selector,
        parentSelector: parent ? describe(parent) : '',
        text: snippet(element),
        whiteSpace: style.whiteSpace,
        textOverflow: style.textOverflow,
        overflowX: style.overflowX,
        width: Math.round(rect.width),
        scrollWidth: element.scrollWidth,
        clientWidth: element.clientWidth,
        right: Math.round(rect.right),
        ...extra
      })
    }

    const wider = element.scrollWidth > element.clientWidth + 2
    const selfScroller = scrollable(element, 'x')

    // 1. content wider than its own box without a usable affordance
    if (wider && !selfScroller && !intentionalClip(element, style)) {
      record(style.overflowX === 'hidden' || style.overflowX === 'clip' ? 'clipped-no-ellipsis' : 'content-wider-than-box')
    }

    // 2. painted past the right edge of the parent's content box
    if (parent && parent !== document.body) {
      const parentStyle = getComputedStyle(parent)
      const parentClips = parentStyle.overflowX === 'hidden' || parentStyle.overflowX === 'clip'
      if (rect.right > contentRight(parent) + 1 && !insideScroller(element, 'x') && !selfScroller &&
          style.position !== 'absolute' && style.position !== 'fixed' && !intentionalClip(element, style)) {
        record(parentClips ? 'clipped-by-parent' : 'spills-past-parent', { parentRight: Math.round(contentRight(parent)) })
      }
    }

    // 3. painted outside the viewport
    if ((rect.right > viewportWidth + 1 || rect.left < -1) && !insideScroller(element, 'x') && !selfScroller &&
        style.position !== 'fixed') {
      record('past-viewport')
    }

    // 4. text clipped by a fixed height with no line clamp
    if (element.scrollHeight > element.clientHeight + 2 &&
        (style.overflowY === 'hidden' || style.overflowY === 'clip') &&
        !intentionalClip(element, style)) {
      record('clipped-vertically')
    }
  }

  return {
    viewportWidth,
    documentScrollWidth: document.documentElement.scrollWidth,
    horizontalPageScroll: document.documentElement.scrollWidth > viewportWidth + 1,
    findings
  }
}

async function login(page, who, landing) {
  await page.goto(`${baseURL}/login`, { waitUntil: 'domcontentloaded' })
  await page.getByPlaceholder('用户名', { exact: true }).fill(credentials[who].username)
  await page.getByPlaceholder('密码', { exact: true }).fill(credentials[who].password)
  await page.getByRole('button', { name: '登录', exact: true }).click()
  await page.waitForURL(new RegExp(`${landing}$`))
  await page.waitForTimeout(600)
}

async function run(browser, { label, viewport, isMobile, who, routes }) {
  await mkdir(path.join(outputDirectory, label), { recursive: true })
  const context = await browser.newContext({ viewport, isMobile: !!isMobile, hasTouch: !!isMobile })
  const page = await context.newPage()
  const consoleErrors = []
  page.on('pageerror', (error) => consoleErrors.push(`${label}: ${error.message}`))
  page.on('console', (message) => {
    if (message.type() === 'error' && !message.text().startsWith('Failed to load resource:')) {
      consoleErrors.push(`${label}: ${message.text()}`)
    }
  })

  await login(page, who, who === 'admin' ? '/admin/home' : '/home')

  const report = []
  for (const [name, route] of routes) {
    await page.goto(`${baseURL}${route}`, { waitUntil: 'domcontentloaded' })
    await page.waitForTimeout(800)
    const result = await page.evaluate(auditPage)
    await page.screenshot({
      path: path.join(outputDirectory, label, `${name}.png`),
      fullPage: true,
      animations: 'disabled'
    })
    report.push({ name, route, ...result })
  }

  await context.close()
  return { label, report, consoleErrors }
}

await mkdir(path.join(outputDirectory, 'desktop'), { recursive: true })
await mkdir(path.join(outputDirectory, 'mobile'), { recursive: true })

const browser = await chromium.launch({ channel: 'msedge', headless: true })
const runs = []

// UI_AUDIT_WIDTHS lets the same audit sweep several viewport widths, which is
// how the breakpoint where a layout starts overflowing is found.
const widths = (process.env.UI_AUDIT_WIDTHS || '1440,390')
  .split(',')
  .map((value) => parseInt(value.trim(), 10))
  .filter((value) => Number.isFinite(value) && value > 0)

for (const width of widths) {
  const label = width <= 480 ? 'mobile' : `w${width}`
  const viewport = { width, height: width <= 480 ? 844 : 960 }
  const isMobile = width <= 480
  runs.push(await run(browser, { label, viewport, isMobile, who: 'student', routes: studentRoutes }))
  runs.push(await run(browser, { label, viewport, isMobile, who: 'admin', routes: adminRoutes }))
}

await browser.close()

const flat = runs.flatMap((r) => r.report.map((p) => ({ ...p, label: r.label })))
await writeFile(path.join(outputDirectory, 'audit.json'), JSON.stringify({ runs: flat, consoleErrors: runs.flatMap((r) => r.consoleErrors) }, null, 2))

const byKindTotal = {}
let total = 0
for (const pageReport of flat) {
  total += pageReport.findings.length
  for (const finding of pageReport.findings) {
    byKindTotal[finding.kind] = (byKindTotal[finding.kind] || 0) + 1
  }
  if (pageReport.findings.length === 0 && !pageReport.horizontalPageScroll) continue
  const flag = pageReport.horizontalPageScroll ? ` PAGE-HSCROLL(doc=${pageReport.documentScrollWidth})` : ''
  console.log(`\n[${pageReport.label}] ${pageReport.name.padEnd(24)} ${pageReport.route.padEnd(20)} findings=${pageReport.findings.length}${flag}`)
  for (const finding of pageReport.findings.slice(0, 6)) {
    console.log(`    ${finding.kind.padEnd(22)} ${finding.selector.slice(0, 38).padEnd(38)} w=${finding.width} sw=${finding.scrollWidth}${finding.parentRight ? ' pr=' + finding.parentRight : ''} | ${finding.text.slice(0, 30)}`)
  }
}

console.log('\n=== findings by kind ===')
for (const [kind, count] of Object.entries(byKindTotal).sort((a, b) => b[1] - a[1])) {
  console.log(`  ${kind.padEnd(24)} ${count}`)
}
console.log(`\nTOTAL findings: ${total}`)
const errors = runs.flatMap((r) => r.consoleErrors)
console.log(`console errors: ${errors.length}`)
errors.slice(0, 8).forEach((e) => console.log('  ' + e))
console.log(`artifacts: ${outputDirectory}`)
