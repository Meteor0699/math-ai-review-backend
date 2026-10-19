<template>
  <div class="login-page">
    <section class="intro-panel">
      <AppLogo class="login-logo" />
      <h1>把期末复习，变成清晰可执行的学习路径</h1>
      <p class="intro-copy">
        从课程章节到题库练习，再到 AI 分步讲解。把高等数学、线性代数和概率统计放进同一个复习工作台。
      </p>
      <div class="course-preview">
        <div v-for="course in coursePreview" :key="course.name" class="preview-item">
          <img :src="course.cover" :alt="`${course.name}教材封面`" />
          <span>{{ course.name }}</span>
        </div>
      </div>
      <ul class="intro-features">
        <li><el-icon><CircleCheck /></el-icon>章节知识点与重点公式集中复习</li>
        <li><el-icon><CircleCheck /></el-icon>题目答案按需查看，保留独立思考</li>
        <li><el-icon><CircleCheck /></el-icon>AI 讲解支持围绕当前题目继续追问</li>
      </ul>
    </section>

    <section class="login-card">
      <div class="login-header">
        <h2>开始学习</h2>
        <p>登录后进入你的数学复习空间</p>
      </div>

      <el-tabs v-model="mode" stretch>
        <el-tab-pane label="登录" name="login">
          <el-form ref="loginFormRef" :model="loginForm" :rules="loginRules" label-width="0" size="large">
            <el-form-item prop="username">
              <el-input v-model="loginForm.username" placeholder="用户名" :prefix-icon="User" clearable />
            </el-form-item>
            <el-form-item prop="password">
              <el-input
                v-model="loginForm.password"
                type="password"
                placeholder="密码"
                :prefix-icon="Lock"
                show-password
                @keyup.enter="handleLogin"
              />
            </el-form-item>
            <el-form-item>
              <el-button type="primary" :loading="loading" class="submit-button" @click="handleLogin">
                {{ loading ? '登录中...' : '登录' }}
              </el-button>
            </el-form-item>
          </el-form>
        </el-tab-pane>

        <el-tab-pane label="学生注册" name="register">
          <el-form ref="registerFormRef" :model="registerForm" :rules="registerRules" label-width="0" size="large">
            <el-form-item prop="username">
              <el-input v-model="registerForm.username" placeholder="设置用户名" :prefix-icon="User" clearable />
            </el-form-item>
            <el-form-item prop="realName">
              <el-input v-model="registerForm.realName" placeholder="真实姓名" :prefix-icon="UserFilled" clearable />
            </el-form-item>
            <el-form-item prop="studentNo">
              <el-input v-model="registerForm.studentNo" placeholder="学号（可选）" :prefix-icon="Tickets" clearable />
            </el-form-item>
            <el-form-item prop="password">
              <el-input
                v-model="registerForm.password"
                type="password"
                placeholder="设置密码，至少 6 位"
                :prefix-icon="Lock"
                show-password
              />
            </el-form-item>
            <el-form-item prop="confirmPassword">
              <el-input
                v-model="registerForm.confirmPassword"
                type="password"
                placeholder="再次输入密码"
                :prefix-icon="Lock"
                show-password
                @keyup.enter="handleRegister"
              />
            </el-form-item>
            <el-form-item>
              <el-button type="primary" :loading="loading" class="submit-button" @click="handleRegister">
                {{ loading ? '注册中...' : '注册并进入' }}
              </el-button>
            </el-form-item>
          </el-form>
        </el-tab-pane>
      </el-tabs>

      <div class="login-footer">账号信息仅用于登录验证，请勿与他人共享密码。</div>
    </section>
  </div>
</template>

<script setup>
import { reactive, ref } from 'vue'
import { useRouter } from 'vue-router'
import { Lock, Tickets, User, UserFilled } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import { login as loginApi, register as registerApi } from '../api/auth'
import AppLogo from '../components/AppLogo.vue'
import { useUserStore } from '../stores/user'

const router = useRouter()
const userStore = useUserStore()
const mode = ref('login')
const loading = ref(false)
const loginFormRef = ref(null)
const registerFormRef = ref(null)

const loginForm = reactive({
  username: '',
  password: ''
})

const registerForm = reactive({
  username: '',
  realName: '',
  studentNo: '',
  password: '',
  confirmPassword: ''
})

const coursePreview = [
  { name: '高等数学', cover: '/textbook-covers/tongji-advanced-math-1.svg' },
  { name: '线性代数', cover: '/textbook-covers/engineering-math-linear-algebra.svg' },
  { name: '概率统计', cover: '/textbook-covers/znufe-probability-statistics.svg' }
]

const loginRules = {
  username: [{ required: true, message: '请输入用户名', trigger: 'blur' }],
  password: [{ required: true, message: '请输入密码', trigger: 'blur' }]
}

const registerRules = {
  username: [
    { required: true, message: '请输入用户名', trigger: 'blur' },
    { min: 2, max: 64, message: '用户名长度为 2-64 位', trigger: 'blur' }
  ],
  realName: [{ required: true, message: '请输入真实姓名', trigger: 'blur' }],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 6, max: 64, message: '密码长度为 6-64 位', trigger: 'blur' }
  ],
  confirmPassword: [
    { required: true, message: '请再次输入密码', trigger: 'blur' },
    {
      validator: (_rule, value, callback) => {
        if (value !== registerForm.password) callback(new Error('两次输入的密码不一致'))
        else callback()
      },
      trigger: 'blur'
    }
  ]
}

function enterApp(user) {
  if (user.role === 'admin') router.push('/admin/home')
  else router.push('/home')
}

async function handleLogin() {
  const valid = await loginFormRef.value.validate().catch(() => false)
  if (!valid) return

  loading.value = true
  try {
    const res = await loginApi(loginForm.username, loginForm.password)
    if (res.code === 200) {
      const { token, user } = res.data
      userStore.setLogin(token, user)
      ElMessage.success(`欢迎回来，${user.realName || user.username}`)
      enterApp(user)
    } else {
      ElMessage.error(res.message || '登录失败')
    }
  } finally {
    loading.value = false
  }
}

async function handleRegister() {
  const valid = await registerFormRef.value.validate().catch(() => false)
  if (!valid) return

  loading.value = true
  try {
    const res = await registerApi({
      username: registerForm.username,
      password: registerForm.password,
      realName: registerForm.realName,
      studentNo: registerForm.studentNo
    })
    if (res.code === 200) {
      const { token, user } = res.data
      userStore.setLogin(token, user)
      ElMessage.success('注册成功')
      enterApp(user)
    } else {
      ElMessage.error(res.message || '注册失败')
    }
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.login-page {
  min-height: 100vh;
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(380px, 450px);
  align-items: center;
  gap: clamp(40px, 7vw, 100px);
  padding: 48px max(36px, 8vw);
  background: var(--color-background);
}

.intro-panel {
  max-width: 680px;
}

.login-logo {
  margin-bottom: 36px;
}

.intro-panel h1 {
  max-width: 620px;
  margin-bottom: 18px;
  color: var(--color-text-primary);
  font-size: 38px;
  line-height: 1.28;
  letter-spacing: 0;
}

.intro-copy {
  font-size: 16px;
  line-height: 1.8;
  color: var(--color-text-secondary);
  max-width: 620px;
}

.course-preview {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14px;
  margin-top: 30px;
}

.preview-item {
  display: flex;
  align-items: center;
  gap: 12px;
  min-width: 0;
  padding: 10px;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  background: var(--color-surface);
}

.preview-item img {
  width: 52px;
  height: 68px;
  flex: 0 0 52px;
  border-radius: var(--radius-sm);
  object-fit: cover;
}

.preview-item span {
  overflow: hidden;
  font-size: var(--text-sm);
  font-weight: 650;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.intro-features {
  display: grid;
  gap: 9px;
  margin-top: 26px;
  color: var(--color-text-secondary);
  list-style: none;
}

.intro-features li {
  display: flex;
  align-items: center;
  gap: 9px;
}

.intro-features .el-icon {
  color: var(--color-success);
}

.login-card {
  width: 100%;
  padding: 32px;
  border-radius: var(--radius-xl);
  background: var(--color-surface);
  box-shadow: var(--shadow-md);
}

.login-header {
  margin-bottom: 18px;
}

.login-header h2 {
  font-size: 24px;
  color: var(--color-text-primary);
  margin-bottom: 6px;
}

.login-header p {
  color: var(--color-text-secondary);
  font-size: 14px;
}

.submit-button {
  width: 100%;
}

.login-footer {
  margin-top: 12px;
  text-align: center;
  font-size: 12px;
  color: var(--color-text-muted);
}

@media (max-width: 900px) {
  .login-page {
    grid-template-columns: 1fr;
    gap: 26px;
    padding: 28px 18px;
  }

  .intro-panel h1 {
    font-size: 30px;
  }

  .course-preview,
  .intro-features {
    display: none;
  }

  .login-card {
    padding: 24px;
  }
}
</style>
