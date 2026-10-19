// Runtime check after switching Element Plus to on-demand imports:
// unresolved components/directives show up as Vue console warnings, so this
// fails loudly if the build is visually broken. Also reports basic DOM sanity.
import { chromium } from '@playwright/test'

const baseURL = process.env.E2E_BASE_URL || 'http://127.0.0.1:8080'
// Only the public login page is exercised here: it renders Element Plus inputs,
// tabs and buttons, which is enough to catch unresolved on-demand components or
// directives. Authenticated routes need a database, so they are not required.

const browser = await chromium.launch({ channel: 'msedge', headless: true })
const context = await browser.newContext({ viewport: { width: 1440, height: 960 } })
const page = await context.newPage()
const problems = []

page.on('console', (msg) => {
  const text = msg.text()
  if (msg.type() === 'error' || msg.type() === 'warning') {
    if (text.startsWith('Failed to load resource')) return
    problems.push(`${msg.type()}: ${text}`)
  }
})
page.on('pageerror', (err) => problems.push(`pageerror: ${err.message}`))

await page.goto(`${baseURL}/login`, { waitUntil: 'domcontentloaded' })
await page.waitForTimeout(1200)

for (const route of ['/login']) {
  await page.goto(`${baseURL}${route}`, { waitUntil: 'domcontentloaded' })
  await page.waitForTimeout(900)
  const stats = await page.evaluate(() => ({
    elComponents: document.querySelectorAll('[class*="el-"]').length,
    buttons: document.querySelectorAll('button').length,
    tables: document.querySelectorAll('.el-table').length,
    dialogs: document.querySelectorAll('.el-dialog, .el-drawer').length,
    inputs: document.querySelectorAll('.el-input').length,
    tabs: document.querySelectorAll('.el-tabs').length,
    text: (document.body.innerText || '').replace(/\s+/g, ' ').slice(0, 60)
  }))
  console.log(`${route.padEnd(16)} el*= ${String(stats.elComponents).padStart(4)} buttons=${stats.buttons} inputs=${stats.inputs} tabs=${stats.tabs} text="${stats.text}"`)
}

await context.close()
await browser.close()

const unresolved = problems.filter((p) => /Failed to resolve|Unhandled error|Cannot read/i.test(p))
console.log(`\nconsole problems: ${problems.length} (unresolved/critical: ${unresolved.length})`)
problems.slice(0, 8).forEach((p) => console.log('  ' + p))
process.exit(unresolved.length > 0 ? 1 : 0)
