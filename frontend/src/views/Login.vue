<template>
  <div class="login-container">
    <div class="login-card">
      <div class="login-header">
        <div class="login-icon">
          <el-icon :size="48"><School /></el-icon>
        </div>
        <h1 class="login-title">数学 AI 复习助手</h1>
        <p class="login-subtitle">高校数学智能复习平台</p>
      </div>

      <el-tabs v-model="mode" stretch>
        <el-tab-pane label="登录" name="login">
          <el-form ref="loginFormRef" :model="loginForm" :rules="loginRules" label-width="0" size="large">
            <el-form-item prop="username">
              <el-input v-model="loginForm.username" placeholder="请输入用户名" :prefix-icon="User" clearable />
            </el-form-item>
            <el-form-item prop="password">
              <el-input
                v-model="loginForm.password"
                type="password"
                placeholder="请输入密码"
                :prefix-icon="Lock"
                show-password
                @keyup.enter="handleLogin"
              />
            </el-form-item>
            <el-form-item>
              <el-button type="primary" :loading="loading" style="width: 100%" @click="handleLogin">
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
              <el-button type="success" :loading="loading" style="width: 100%" @click="handleRegister">
                {{ loading ? '注册中...' : '注册并进入' }}
              </el-button>
            </el-form-item>
          </el-form>
        </el-tab-pane>
      </el-tabs>

      <div class="login-footer">
        <span>测试账号：admin / 123456，student / 123456</span>
      </div>
    </div>
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
.login-container {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 24px;
  background:
    radial-gradient(circle at 18% 18%, rgba(64, 158, 255, 0.16), transparent 32%),
    linear-gradient(135deg, #eef6ff 0%, #f8fbf6 48%, #fff7ed 100%);
}

.login-card {
  width: min(440px, 100%);
  padding: 34px 36px 28px;
  border-radius: 10px;
  background: rgba(255, 255, 255, 0.96);
  box-shadow: 0 18px 50px rgba(31, 45, 61, 0.14);
}

.login-header {
  text-align: center;
  margin-bottom: 22px;
}

.login-icon {
  width: 72px;
  height: 72px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border-radius: 50%;
  color: #fff;
  background: linear-gradient(135deg, #3078c6, #54b65f);
  margin-bottom: 14px;
}

.login-title {
  font-size: 24px;
  font-weight: 700;
  color: #1f2d3d;
  margin-bottom: 6px;
}

.login-subtitle {
  font-size: 14px;
  color: #6b7280;
}

.login-footer {
  margin-top: 12px;
  text-align: center;
  font-size: 12px;
  color: #909399;
}
</style>
