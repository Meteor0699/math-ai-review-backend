import { chromium } from '@playwright/test'
import { mkdir } from 'node:fs/promises'
import path from 'node:path'

const baseURL = process.env.E2E_BASE_URL || 'http://127.0.0.1:8080'
const outputDirectory = process.env.UI_SCREENSHOT_DIR
const studentUsername = process.env.E2E_STUDENT_USERNAME
const studentPassword = process.env.E2E_STUDENT_PASSWORD
const adminUsername = process.env.E2E_ADMIN_USERNAME
const adminPassword = process.env.E2E_ADMIN_PASSWORD

if (!outputDirectory || !studentUsername || !studentPassword || !adminUsername || !adminPassword) {
  throw new Error('UI screenshot directory and E2E credentials are required')
}

await mkdir(outputDirectory, { recursive: true })

const browser = await chromium.launch({ channel: 'msedge', headless: true })
const consoleErrors = []

function observe(page, label) {
  page.on('pageerror', (error) => consoleErrors.push(`${label}: ${error.message}`))
  page.on('console', (message) => {
    if (message.type() === 'error' && !message.text().startsWith('Failed to load resource:')) {
      consoleErrors.push(`${label}: ${message.text()}`)
    }
  })
  page.on('response', (response) => {
    if (response.status() >= 400) {
      consoleErrors.push(`${label}: HTTP ${response.status()} ${response.url()}`)
    }
  })
}

async function openAndSettle(page, route) {
  await page.goto(`${baseURL}${route}`, { waitUntil: 'domcontentloaded' })
  await page.waitForTimeout(700)
}

async function screenshot(page, filename) {
  await page.screenshot({
    path: path.join(outputDirectory, filename),
    fullPage: true,
    animations: 'disabled'
  })
}

async function login(page, username, password, expectedRoute) {
  await openAndSettle(page, '/login')
  await page.getByPlaceholder('用户名', { exact: true }).fill(username)
  await page.getByPlaceholder('密码', { exact: true }).fill(password)
  await page.getByRole('button', { name: '登录', exact: true }).click()
  await page.waitForURL(new RegExp(`${expectedRoute}$`))
  await page.waitForTimeout(700)
}

const desktop = await browser.newContext({ viewport: { width: 1440, height: 960 } })
const desktopPage = await desktop.newPage()
observe(desktopPage, 'desktop')

await openAndSettle(desktopPage, '/login')
await screenshot(desktopPage, 'login.png')

await login(desktopPage, studentUsername, studentPassword, '/home')
await screenshot(desktopPage, 'student-home.png')

await openAndSettle(desktopPage, '/courses')
await screenshot(desktopPage, 'courses.png')

await openAndSettle(desktopPage, '/questions')
await screenshot(desktopPage, 'questions.png')

await openAndSettle(desktopPage, '/questions/1')
await screenshot(desktopPage, 'question-detail.png')

await openAndSettle(desktopPage, '/study')
await screenshot(desktopPage, 'study-center.png')

await desktop.close()

const admin = await browser.newContext({ viewport: { width: 1440, height: 960 } })
const adminPage = await admin.newPage()
observe(adminPage, 'admin')
await login(adminPage, adminUsername, adminPassword, '/admin/home')
await screenshot(adminPage, 'admin-home.png')
await openAndSettle(adminPage, '/admin/users')
await screenshot(adminPage, 'user-management.png')
await admin.close()

const mobile = await browser.newContext({
  viewport: { width: 390, height: 844 },
  isMobile: true,
  hasTouch: true
})
const mobilePage = await mobile.newPage()
observe(mobilePage, 'mobile')
await login(mobilePage, studentUsername, studentPassword, '/home')
await screenshot(mobilePage, 'mobile-home.png')
await openAndSettle(mobilePage, '/questions/1')
await screenshot(mobilePage, 'mobile-question-detail.png')
await mobile.close()

await browser.close()

if (consoleErrors.length > 0) {
  throw new Error(`Browser console errors:\n${consoleErrors.join('\n')}`)
}

console.log(`Saved UI screenshots to ${outputDirectory}`)
