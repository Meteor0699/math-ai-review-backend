// Accessibility regression test (Front-End Checklist `accessibility-testing`).
// Scans the public gate plus the main authenticated surfaces. Run with:
//   E2E_BASE_URL=http://127.0.0.1:8080 E2E_STUDENT_USERNAME=student \
//   E2E_STUDENT_PASSWORD=... npx playwright test e2e/a11y.spec.js
import { test, expect } from '@playwright/test'
import AxeBuilder from '@axe-core/playwright'

const student = {
  username: process.env.E2E_STUDENT_USERNAME,
  password: process.env.E2E_STUDENT_PASSWORD
}

// WCAG A/AA only: the two clickable <article role="link"> rows trip axe's
// best-practice aria-allowed-role rule, which is not a WCAG failure.
const TAGS = ['wcag2a', 'wcag2aa', 'wcag21a', 'wcag21aa']

async function scan(page, label) {
  const results = await new AxeBuilder({ page }).withTags(TAGS).analyze()
  const summary = results.violations.map((v) => ({
    id: v.id,
    impact: v.impact,
    help: v.help,
    nodes: v.nodes.slice(0, 4).map((n) => ({
      target: n.target.join(' '),
      html: (n.html || '').slice(0, 140),
      // axe reports the measured colours for contrast checks; print them so the
      // fix is a known token change rather than a guess.
      contrast: n.any?.[0]?.data
        ? `${n.any[0].data.fgColor} on ${n.any[0].data.bgColor} = ${n.any[0].data.contrastRatio}`
        : undefined
    }))
  }))
  expect(summary, `${label} axe violations: ${JSON.stringify(summary, null, 2)}`).toEqual([])
}

test('login page has no WCAG A/AA violations', async ({ page }) => {
  await page.goto('/login')
  await page.waitForTimeout(600)
  await scan(page, 'login')
})

test('student pages have no WCAG A/AA violations', async ({ page }) => {
  test.skip(!student.username, 'E2E student credentials are required')
  await page.goto('/login')
  await page.getByPlaceholder('用户名', { exact: true }).fill(student.username)
  await page.getByPlaceholder('密码', { exact: true }).fill(student.password)
  await page.getByRole('button', { name: '登录', exact: true }).click()
  await page.waitForURL(/\/home$/)

  for (const route of ['/home', '/courses', '/questions', '/study']) {
    await page.goto(route)
    await page.waitForTimeout(700)
    await scan(page, route)
  }
})
