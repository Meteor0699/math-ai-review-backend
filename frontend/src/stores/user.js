import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { clearAuthStorage, readToken, readUser, writeAuthStorage } from '../utils/authStorage'

export const useUserStore = defineStore('user', () => {
  const token = ref(readToken())
  const user = ref(readUser())

  const isLoggedIn = computed(() => !!token.value)
  const isAdmin = computed(() => user.value?.role === 'admin')
  const username = computed(() => user.value?.username || '')
  const realName = computed(() => user.value?.realName || '')

  /** 直接设置登录状态（由调用方校验后调用） */
  function setLogin(newToken, newUser) {
    token.value = newToken
    user.value = newUser
    writeAuthStorage(newToken, newUser)
  }

  function logout() {
    token.value = ''
    user.value = null
    clearAuthStorage()
  }

  return {
    token,
    user,
    isLoggedIn,
    isAdmin,
    username,
    realName,
    setLogin,
    logout
  }
})
