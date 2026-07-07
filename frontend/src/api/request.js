import axios from 'axios'
import { ElMessage } from 'element-plus'

const request = axios.create({
  baseURL: import.meta.env.VITE_API_BASE_URL || '/api',
  timeout: 30000
})

// 请求拦截器：自动携带 token
request.interceptors.request.use(
  (config) => {
    const token = localStorage.getItem('token')
    if (token) {
      config.headers.Authorization = `Bearer ${token}`
    }
    return config
  },
  (error) => Promise.reject(error)
)

// 响应拦截器：统一错误处理
request.interceptors.response.use(
  (response) => {
    const res = response.data
    // 登录接口 code 可能不是 200，不在拦截器中统一弹窗
    if (res.code && res.code !== 200 && response.config.url !== '/auth/login') {
      ElMessage.error(res.message || '请求失败')
    }
    return res
  },
  (error) => {
    if (error.response) {
      const status = error.response.status
      if (status === 401) {
        // 登录接口的 401 由页面自行处理（如“用户名或密码错误”），
        // 不在此处当作“登录过期”处理
        if (error.config.url === '/auth/login') {
          ElMessage.error(error.response.data?.message || '用户名或密码错误')
        } else {
          localStorage.removeItem('token')
          localStorage.removeItem('user')
          window.location.href = '/login'
          ElMessage.error('登录已过期，请重新登录')
        }
      } else {
        ElMessage.error(error.response.data?.message || '服务器错误')
      }
    } else {
      ElMessage.error('网络错误，请检查网络连接')
    }
    return Promise.reject(error)
  }
)

export default request
