<template>
  <div class="login-page">
    <section class="intro-panel">
      <div class="brand-row">
        <span class="brand-mark">
          <el-icon><School /></el-icon>
        </span>
        <span>数研 AI</span>
      </div>
      <h1>把大学数学复习拆成每天能完成的小任务</h1>
      <p class="intro-copy">
        课程教材、章节知识点、题库和 AI 讲解放在同一个学习空间里，适合期中、期末和考前集中复盘。
      </p>
      <div class="intro-stats">
        <div>
          <strong>3</strong>
          <span>门核心课程</span>
        </div>
        <div>
          <strong>AI</strong>
          <span>题目讲解与追问</span>
        </div>
        <div>
          <strong>历年</strong>
          <span>试题资料管理</span>
        </div>
      </div>
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

      <div class="login-footer">
        测试账号：admin / 123456，student / 123456
      </div>
    </section>
  </div>
</template>

<script setup>
import { reactive, ref } from 'vue'
import { useRouter } from 'vue-router'
import { Lock, School, Tickets, User, UserFilled } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import { login as loginApi, register as registerApi } from '../api/auth'
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
  grid-template-columns: minmax(0, 1.1fr) minmax(380px, 460px);
  align-items: center;
  gap: 48px;
  padding: 48px max(36px, 8vw);
  background:
    linear-gradient(180deg, rgba(234, 242, 255, 0.78), rgba(246, 248, 251, 0.25)),
    #f6f8fb;
}

.intro-panel {
  max-width: 620px;
}

.brand-row {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  color: #2563eb;
  font-weight: 700;
  margin-bottom: 28px;
}

.brand-mark {
  width: 38px;
  height: 38px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border-radius: 8px;
  background: #eaf2ff;
}

.intro-panel h1 {
  font-size: 42px;
  line-height: 1.2;
  color: #1f2937;
  max-width: 560px;
  margin-bottom: 18px;
}

.intro-copy {
  font-size: 16px;
  line-height: 1.8;
  color: #5b6472;
  max-width: 560px;
}

.intro-stats {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12px;
  margin-top: 34px;
}

.intro-stats div {
  padding: 18px;
  border: 1px solid #dbe6f8;
  border-radius: 8px;
  background: rgba(255, 255, 255, 0.72);
}

.intro-stats strong {
  display: block;
  font-size: 24px;
  color: #2563eb;
  margin-bottom: 6px;
}

.intro-stats span {
  color: #6b7280;
  font-size: 13px;
}

.login-card {
  width: 100%;
  padding: 32px;
  border-radius: 8px;
  background: #fff;
  border: 1px solid #e5e7eb;
  box-shadow: 0 18px 48px rgba(31, 41, 55, 0.10);
}

.login-header {
  margin-bottom: 18px;
}

.login-header h2 {
  font-size: 24px;
  color: #1f2937;
  margin-bottom: 6px;
}

.login-header p {
  color: #6b7280;
  font-size: 14px;
}

.submit-button {
  width: 100%;
}

.login-footer {
  margin-top: 12px;
  text-align: center;
  font-size: 12px;
  color: #9ca3af;
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

  .intro-stats {
    grid-template-columns: 1fr;
  }

  .login-card {
    padding: 24px;
  }
}
</style>
