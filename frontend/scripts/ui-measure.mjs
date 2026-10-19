// Measure the real box geometry of selected elements on one route.
// Usage: MEASURE_SELECTORS=".a,.b" MEASURE_ROUTE=/questions/1 node scripts/ui-measure.mjs
import { chromium } from '@playwright/test'

const baseURL = process.env.E2E_BASE_URL || 'http://127.0.0.1:5173'
const route = process.env.MEASURE_ROUTE || '/questions/1'
const role = process.env.MEASURE_ROLE || 'student'
const selectors = (process.env.MEASURE_SELECTORS || 'body').split(',').map((s) => s.trim()).filter(Boolean)
const credentials = role === 'admin'
  ? { username: process.env.E2E_ADMIN_USERNAME, password: process.env.E2E_ADMIN_PASSWORD }
  : { username: process.env.E2E_STUDENT_USERNAME, password: process.env.E2E_STUDENT_PASSWORD }

const browser = await chromium.launch({ channel: 'msedge', headless: true })
const context = await browser.newContext({ viewport: { width: 1440, height: 960 } })
const page = await context.newPage()

await page.goto(`${baseURL}/login`, { waitUntil: 'domcontentloaded' })
await page.getByPlaceholder('用户名', { exact: true }).fill(credentials.username)
await page.getByPlaceholder('密码', { exact: true }).fill(credentials.password)
await page.getByRole('button', { name: '登录', exact: true }).click()
await page.waitForURL(new RegExp(role === 'admin' ? '/admin/home$' : '/home$'))
await page.goto(`${baseURL}${route}`, { waitUntil: 'domcontentloaded' })
await page.waitForTimeout(900)

const rows = await page.evaluate((list) => {
  const out = []
  for (const selector of list) {
    const nodes = Array.from(document.querySelectorAll(selector))
    if (!nodes.length) {
      out.push({ selector, count: 0 })
      continue
    }
    const describe = (el) => {
      const r = el.getBoundingClientRect()
      const s = getComputedStyle(el)
      return {
        h: Math.round(r.height),
        w: Math.round(r.width),
        minHeight: s.minHeight,
        padding: `${s.paddingTop} ${s.paddingRight} ${s.paddingBottom} ${s.paddingLeft}`,
        margin: `${s.marginTop} ${s.marginBottom}`,
        gap: s.gap,
        fontSize: s.fontSize,
        lineHeight: s.lineHeight,
        display: s.display
      }
    }
    const all = nodes.map(describe)
    out.push({
      selector,
      count: nodes.length,
      first: all[0],
      min: Math.min(...all.map((a) => a.h)),
      max: Math.max(...all.map((a) => a.h)),
      totalHeight: all.reduce((sum, a) => sum + a.h, 0)
    })
  }
  return out
}, selectors)

console.log(`route=${route} role=${role}`)
for (const row of rows) {
  if (row.count === 0) {
    console.log(`  ${row.selector}  -> NOT FOUND`)
    continue
  }
  console.log(`  ${row.selector}`)
  console.log(`     count=${row.count} height first=${row.first.h} min=${row.min} max=${row.max} total=${row.totalHeight}`)
  console.log(`     min-height=${row.first.minHeight} padding=${row.first.padding} gap=${row.first.gap} font=${row.first.fontSize}/${row.first.lineHeight} display=${row.first.display}`)
}

await context.close()
await browser.close()
