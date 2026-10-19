import request from './request'
import { readToken } from '../utils/authStorage'

// Agent 模式接口（见 docs/AGENT_MODE.md）。
// 会话恢复走 axios（复用统一错误处理），流式对话必须走 fetch + ReadableStream。

// 契约路径见 docs/AGENT_MODE.md（/api/v1/...）。axios 实例的 baseURL 是 '/api'，
// 所以这里补上 '/v1'；fetch 那一侧用 API_BASE + '/v1' 保持同一条路径。
const AGENT_API_VERSION = '/v1'

export function getAgentSession(questionId) {
  return request({
    url: `${AGENT_API_VERSION}/questions/${questionId}/agent/session`,
    method: 'get'
  })
}

// baseURL 与 axios 实例保持一致：开发环境为 '/api'，可由 VITE_API_BASE_URL 覆盖。
function resolveApiBase() {
  const configured = import.meta.env.VITE_API_BASE_URL || '/api'
  const base = configured.replace(/\/+$/, '')
  if (base.endsWith(AGENT_API_VERSION)) return base
  return `${base}${AGENT_API_VERSION}`
}

// 401/403 交给 axios 拦截器那一套处理：清空登录态并跳转登录页。
function handleUnauthorized() {
  localStorage.removeItem('token')
  localStorage.removeItem('user')
  if (typeof window !== 'undefined') {
    window.location.href = '/login'
  }
}

// 把 SSE 单个事件帧（已去掉结尾空行）解析成 { event, data }。
// 域名前缀（event:/data:）在解析时被丢弃，只保留 data 文本。
export function parseSseFrame(rawFrame) {
  const frame = String(rawFrame ?? '').replace(/\r\n/g, '\n').replace(/\r/g, '\n')
  if (!frame.trim()) return null
  let eventName = 'message'
  const dataLines = []
  for (const line of frame.split('\n')) {
    if (!line || line.startsWith(':')) continue
    const separator = line.indexOf(':')
    const field = separator === -1 ? line : line.slice(0, separator)
    let value = separator === -1 ? '' : line.slice(separator + 1)
    if (value.startsWith(' ')) value = value.slice(1)
    if (field === 'event') eventName = value.trim()
    else if (field === 'data') dataLines.push(value)
  }
  if (!dataLines.length) return null
  const dataText = dataLines.join('\n')
  let data
  try {
    data = JSON.parse(dataText)
  } catch {
    data = dataText
  }
  return { event: eventName, data }
}

/**
 * 发起一轮 agent 对话，按 SSE 帧回调 onEvent({ event, data })。
 * 返回 { aborted }；HTTP/网络错误会抛出 Error。
 */
export async function streamAgentChat(questionId, { message, conversationId, signal, onEvent, extraHeaders }) {
  const headers = {
    Accept: 'text/event-stream',
    'Content-Type': 'application/json'
  }
  const token = readToken()
  if (token) {
    headers.Authorization = `Bearer ${token}`
  }
  Object.assign(headers, extraHeaders || {})

  const response = await fetch(`${resolveApiBase()}/questions/${questionId}/agent/chat`, {
    method: 'POST',
    headers,
    body: JSON.stringify({ message, conversationId }),
    signal
  })

  if (response.status === 401 || response.status === 403) {
    handleUnauthorized()
    const unauthorized = new Error('登录已过期，请重新登录')
    unauthorized.code = 'unauthorized'
    throw unauthorized
  }

  if (!response.ok) {
    // 失败时后端通常返回 JSON { code, message }，能读就读，读不到退回状态码。
    let detail
    let code = ''
    try {
      const payload = await response.json()
      detail = payload?.message || ''
      code = payload?.code ? String(payload.code) : ''
    } catch {
      detail = ''
    }
    const failure = new Error(detail || `请求失败（HTTP ${response.status}）`)
    failure.code = code || `http_${response.status}`
    throw failure
  }

  if (!response.body) {
    throw new Error('当前浏览器不支持流式响应')
  }

  const reader = response.body.getReader()
  const decoder = new TextDecoder('utf-8')
  let buffer = ''
  try {
    for (;;) {
      const { done, value } = await reader.read()
      if (done) break
      buffer += decoder.decode(value, { stream: true })
      buffer = buffer.replace(/\r\n/g, '\n')
      let boundary = buffer.indexOf('\n\n')
      while (boundary !== -1) {
        const frame = buffer.slice(0, boundary)
        buffer = buffer.slice(boundary + 2)
        const parsed = parseSseFrame(frame)
        if (parsed) onEvent(parsed)
        boundary = buffer.indexOf('\n\n')
      }
    }
    buffer += decoder.decode()
    const tail = parseSseFrame(buffer.replace(/\n+$/, ''))
    if (tail) onEvent(tail)
  } finally {
    try {
      reader.releaseLock()
    } catch {
      // 流已被 abort 时 releaseLock 可能抛错，忽略。
    }
  }

  return { aborted: false }
}
