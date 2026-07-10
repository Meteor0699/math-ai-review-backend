const TOKEN_KEY = 'token'
const USER_KEY = 'user'

export function readToken() {
  return localStorage.getItem(TOKEN_KEY) || ''
}

export function readUser() {
  const raw = localStorage.getItem(USER_KEY)
  if (!raw) return null
  try {
    const user = JSON.parse(raw)
    return user && typeof user === 'object' ? user : null
  } catch {
    clearAuthStorage()
    return null
  }
}

export function writeAuthStorage(token, user) {
  localStorage.setItem(TOKEN_KEY, token)
  localStorage.setItem(USER_KEY, JSON.stringify(user))
}

export function clearAuthStorage() {
  localStorage.removeItem(TOKEN_KEY)
  localStorage.removeItem(USER_KEY)
}
