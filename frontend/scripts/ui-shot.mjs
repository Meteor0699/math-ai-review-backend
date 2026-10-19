// Zoom in on one element: element screenshot at 3x plus its computed geometry.
// Usage: SHOT_SELECTOR=".profile-button" SHOT_ROUTE=/home node scripts/ui-shot.mjs
// SHOT_WIDTH (default 1440) sets the viewport width; a width <= 480 also
// enables the mobile viewport flags so the narrow layout is exercised.
import { chromium } from '@playwright/test'
import path from 'node:path'

const baseURL = process.env.E2E_BASE_URL || 'http://localhost:5173'
const route = process.env.SHOT_ROUTE || '/home'
const selector = process.env.SHOT_SELECTOR || '.profile-button'
const out = process.env.SHOT_OUT || path.resolve('ui-shot.png')
const width = Number.parseInt(process.env.SHOT_WIDTH || '1440', 10)
const height = Number.parseInt(process.env.SHOT_HEIGHT || (width <= 480 ? '844' : '960'), 10)
const role = process.env.SHOT_ROLE || 'student'
const credentials = role === 'admin'
  ? { username: process.env.E2E_ADMIN_USERNAME, password: process.env.E2E_ADMIN_PASSWORD }
  : { username: process.env.E2E_STUDENT_USERNAME, password: process.env.E2E_STUDENT_PASSWORD }

const browser = await chromium.launch({ channel: 'msedge', headless: true })
const context = await browser.newContext({
  viewport: { width, height },
  deviceScaleFactor: 3,
  isMobile: width <= 480,
  hasTouch: width <= 480
})
const page = await context.newPage()

await page.goto(`${baseURL}/login`, { waitUntil: 'domcontentloaded' })
await page.getByPlaceholder('用户名', { exact: true }).fill(credentials.username)
await page.getByPlaceholder('密码', { exact: true }).fill(credentials.password)
await page.getByRole('button', { name: '登录', exact: true }).click()
await page.waitForURL(new RegExp(role === 'admin' ? '/admin/home$' : '/home$'))
await page.goto(`${baseURL}${route}`, { waitUntil: 'domcontentloaded' })
await page.waitForTimeout(800)

const element = page.locator(selector).first()
await element.screenshot({ path: out, animations: 'disabled' })

const info = await page.evaluate((sel) => {
  const node = document.querySelector(sel)
  if (!node) return { missing: true }
  const walk = (el, depth) => {
    const rect = el.getBoundingClientRect()
    const style = getComputedStyle(el)
    return {
      depth,
      tag: el.tagName.toLowerCase() + (typeof el.className === 'string' && el.className ? '.' + el.className.split(/\s+/).slice(0, 2).join('.') : ''),
      text: (el.textContent || '').replace(/\s+/g, ' ').trim().slice(0, 30),
      box: `${Math.round(rect.width)}x${Math.round(rect.height)}`,
      font: `${style.fontSize}/${style.lineHeight} w${style.fontWeight}`,
      padding: style.padding,
      margin: style.margin,
      overflow: style.overflow,
      whiteSpace: style.whiteSpace,
      scroll: `${el.scrollWidth}x${el.scrollHeight}`,
      clipped: el.scrollWidth > el.clientWidth + 1 || el.scrollHeight > el.clientHeight + 1,
      children: depth > 0 ? Array.from(el.children).map((child) => walk(child, depth - 1)) : []
    }
  }
  return walk(node, 2)
}, selector)

const pageOverflow = await page.evaluate(() => ({
  innerWidth: window.innerWidth,
  documentScrollWidth: document.documentElement.scrollWidth,
  horizontalPageScroll: document.documentElement.scrollWidth > window.innerWidth + 1
}))

console.log(`route=${route} selector=${selector} width=${width} -> ${out}`)
console.log(JSON.stringify({ ...info, pageOverflow }, null, 2))

await context.close()
await browser.close()
