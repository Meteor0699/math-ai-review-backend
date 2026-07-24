import { expect, test } from '@playwright/test'

const studentCredentials = {
  username: process.env.E2E_STUDENT_USERNAME,
  password: process.env.E2E_STUDENT_PASSWORD
}

const adminCredentials = {
  username: process.env.E2E_ADMIN_USERNAME,
  password: process.env.E2E_ADMIN_PASSWORD
}

async function login(page, credentials) {
  await page.goto('/login')
  await page.getByPlaceholder('用户名', { exact: true }).fill(credentials.username)
  await page.getByPlaceholder('密码', { exact: true }).fill(credentials.password)
  await page.getByRole('button', { name: '登录', exact: true }).click()
}

test('登录页可正常打开且不公开固定测试密码', async ({ page }) => {
  await page.goto('/login')
  await expect(page.getByRole('heading', { name: '开始学习' })).toBeVisible()
  await expect(page.getByPlaceholder('用户名', { exact: true })).toBeVisible()
  await expect(page.getByText('admin / 123456')).toHaveCount(0)
  await expect(page.getByText('student / 123456')).toHaveCount(0)
})

test.describe('学生核心流程', () => {
  test.skip(!studentCredentials.username || !studentCredentials.password, '需要 E2E 学生账号环境变量')

  test('登录后可打开课程、题库、题目详情、AI 面板和学习中心', async ({ page }, testInfo) => {
    test.skip(testInfo.project.name.includes('mobile'), '完整学生流程由桌面项目验证')
    await login(page, studentCredentials)
    await expect(page).toHaveURL(/\/home$/)
    await expect(page.getByRole('heading', { name: '今天从一个知识点开始' })).toBeVisible()

    await page.goto('/courses')
    await expect(page.getByRole('heading', { name: '课程学习' })).toBeVisible()

    await page.goto('/questions')
    await expect(page.getByRole('heading', { name: '题库练习' })).toBeVisible()

    const firstQuestion = page.locator('.question-card').first()
    await expect(firstQuestion).toBeVisible()
    await firstQuestion.click()
    await expect(page).toHaveURL(/\/questions\/\d+$/)
    await expect(page.getByRole('heading', { name: 'AI 题目讲解' })).toBeVisible()

    let answerRequests = 0
    page.on('request', (request) => {
      if (/\/api\/questions\/\d+\/answer(?:\?|$)/.test(request.url())) answerRequests += 1
    })
    expect(answerRequests).toBe(0)
    await page.getByRole('button', { name: '查看答案与解析' }).click()
    await expect(page.locator('.answer-content h3').first()).toContainText('标准答案')
    expect(answerRequests).toBe(1)

    await page.goto('/study')
    await expect(page.getByRole('heading', { name: '学习中心' })).toBeVisible()
  })

  test('手机视口提供可触控的底部导航', async ({ page }, testInfo) => {
    test.skip(!testInfo.project.name.includes('mobile'), '仅在手机项目验证')
    await login(page, studentCredentials)
    await expect(page.locator('.mobile-nav')).toBeVisible()
    await expect(page.locator('.mobile-nav a')).toHaveCount(5)
    await page.locator('.mobile-nav a[href="/questions"]').click()
    await expect(page).toHaveURL(/\/questions$/)
  })
})

test.describe('管理员核心流程', () => {
  test.skip(!adminCredentials.username || !adminCredentials.password, '需要 E2E 管理员账号环境变量')

  test('管理员可进入仪表盘和用户管理', async ({ page }, testInfo) => {
    test.skip(testInfo.project.name.includes('mobile'), '完整管理员流程由桌面项目验证')
    await login(page, adminCredentials)
    await expect(page).toHaveURL(/\/admin\/home$/)
    await expect(page.getByRole('heading', { name: '数据概览', exact: true })).toBeVisible()
    await page.goto('/admin/users')
    await expect(page.getByRole('heading', { name: '用户管理' })).toBeVisible()
  })
})
