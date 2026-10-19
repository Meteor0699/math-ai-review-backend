<template>
  <section class="agent-panel" aria-labelledby="agent-panel-title">
    <header class="agent-header">
      <span class="agent-icon" aria-hidden="true">AI</span>
      <div class="agent-heading">
        <h2 id="agent-panel-title">AI 学习助手</h2>
        <p>可以连续追问，也能让它查知识点、找同类题、翻你的错题本。</p>
      </div>
      <span class="agent-state" :class="stateModifier">{{ stateLabel }}</span>
    </header>

    <!-- 首屏加载：会话历史骨架 -->
    <div v-if="sessionLoading" class="agent-skeleton" aria-busy="true" aria-label="正在读取历史对话">
      <el-skeleton :rows="3" animated />
    </div>

    <template v-else>
      <div ref="scrollRef" class="agent-stream" role="log" aria-live="polite" aria-relevant="additions text">
        <!-- 空历史：邀请语 + 示例问题 -->
        <div v-if="!messages.length && !sending" class="agent-invite">
          <p class="agent-invite-title">还没有对话，从下面这些问法开始吧。</p>
          <div class="agent-chips">
            <button v-for="example in examples" :key="example" type="button" class="agent-chip" @click="useExample(example)">
              {{ example }}
            </button>
          </div>
        </div>

        <div v-for="message in messages" :key="message.key" class="agent-turn">
          <!-- 用户气泡 -->
          <div v-if="message.role === 'user'" class="agent-row user-row">
            <span class="agent-avatar user-avatar" aria-hidden="true">我</span>
            <div class="agent-bubble user-bubble">
              <p class="agent-plain">{{ message.content }}</p>
            </div>
          </div>

          <!-- 助手正文：唯一允许渲染富文本的地方 -->
          <div v-else-if="message.role === 'assistant'" class="agent-row assistant-row">
            <span class="agent-avatar assistant-avatar" aria-hidden="true">AI</span>
            <div class="agent-bubble assistant-bubble" :data-message-id="message.id ?? undefined">
              <FormulaContent v-if="message.content" :content="message.content" />
              <span v-else-if="message.status === 'streaming'" class="agent-thinking">
                <span class="agent-dot" aria-hidden="true"></span>思考中…
              </span>
              <span v-else-if="message.status === 'failed'" class="agent-note failed-note">本轮生成失败</span>
              <span v-else class="agent-note">已停止生成</span>
              <span v-if="message.status === 'stopped' && message.content" class="agent-note">（已停止生成）</span>
            </div>
          </div>

          <!-- 工具调用卡片 -->
          <div v-else-if="message.role === 'tool'" class="agent-row tool-row">
            <div class="agent-tool" :class="{ running: message.status === 'running' }">
              <div class="agent-tool-head">
                <span class="agent-tool-label">{{ toolLabel(message.toolName) }}</span>
                <span class="agent-tool-meta">
                  {{ message.status === 'running' ? '运行中…' : '已完成' }}<template v-if="Number.isFinite(message.ms)"> · {{ message.ms }}ms</template>
                </span>
              </div>
              <details class="agent-tool-detail">
                <summary>查看调用详情</summary>
                <p class="agent-tool-caption">参数</p>
                <pre class="agent-tool-pre">{{ formatToolText(message.args) }}</pre>
                <template v-if="message.status !== 'running'">
                  <p class="agent-tool-caption">结果</p>
                  <pre class="agent-tool-pre">{{ message.result || '（无结果）' }}</pre>
                </template>
              </details>
            </div>
          </div>
        </div>
      </div>

      <!-- 会话读取失败 -->
      <div v-if="sessionErrorText" class="agent-error" role="alert">
        <span class="agent-error-text">{{ sessionErrorText }}</span>
        <div class="agent-error-actions">
          <button type="button" class="agent-btn" :disabled="sessionLoading" @click="loadSession">重新读取</button>
          <button type="button" class="agent-btn ghost" @click="sessionErrorText = ''">知道了</button>
        </div>
      </div>

      <!-- 本轮失败提示 + 重试 -->
      <div v-if="errorText" class="agent-error" role="alert">
        <span class="agent-error-text">{{ errorText }}</span>
        <div class="agent-error-actions">
          <button type="button" class="agent-btn" :disabled="sending" @click="retryLast">重试</button>
          <button type="button" class="agent-btn ghost" @click="dismissError">知道了</button>
        </div>
      </div>

      <!-- 停止后的恢复入口：历史里最后一条是用户消息时 -->
      <div v-if="!sending && !sessionErrorText && awaitingReply && lastUserText" class="agent-resume">
        <button type="button" class="agent-btn" :disabled="!lastUserText" @click="retryLast">继续生成</button>
      </div>

      <!-- 输入区 -->
      <div class="agent-composer">
        <textarea
          id="agent-input"
          ref="inputRef"
          v-model="draft"
          class="agent-input"
          rows="2"
          maxlength="1000"
          aria-label="向 AI 学习助手提问"
          :disabled="sending"
          placeholder="继续追问这道题，例如：为什么第二步要提出公因式？"
          @keydown="onKeydown"
        ></textarea>
        <div class="agent-composer-foot">
          <span class="agent-hint">Enter 发送，Shift + Enter 换行</span>
          <div class="agent-composer-actions">
            <button
              v-if="sending"
              type="button"
              class="agent-btn danger"
              aria-label="停止生成"
              @click="stop"
            >
              停止
            </button>
            <button
              type="button"
              class="agent-btn primary"
              aria-label="发送消息"
              :disabled="!canSend"
              @click="send()"
            >
              发送
            </button>
          </div>
        </div>
      </div>
    </template>
  </section>
</template>

<script setup>
import { computed, nextTick, onBeforeUnmount, onMounted, ref } from 'vue'
import FormulaContent from './FormulaContent.vue'
import { getAgentSession, streamAgentChat } from '../api/agent'

const props = defineProps({
  questionId: { type: [String, Number], required: true }
})

const examples = ['这题考什么知识点', '为什么要先约分', '给我一道同类题练手']

// 6 个工具的展示名，未知工具名原样展示。
const TOOL_LABELS = {
  search_questions: '搜索题库',
  get_question_answer: '读取本题答案',
  list_knowledge_points: '查询知识点',
  list_my_wrong_questions: '查看我的错题本',
  list_my_study_records: '查看我的学习记录',
  build_practice_set: '生成练习题'
}

const messages = ref([])
const draft = ref('')
const sessionLoading = ref(false)
const sending = ref(false)
const errorText = ref('')
const sessionErrorText = ref('')
const conversationId = ref(null)
const assistantMessageId = ref(null)
const lastUserText = ref('')
const scrollRef = ref(null)
const inputRef = ref(null)

let controller = null
let messageSeq = 0

const canSend = computed(() => !sending.value && draft.value.trim().length > 0)
const currentAssistant = computed(() => messages.value.find((message) => message.role === 'assistant' && message.status === 'streaming') || null)
const awaitingReply = computed(() => messages.value.length > 0 && messages.value[messages.value.length - 1].role === 'user')
const stateLabel = computed(() => {
  if (sessionLoading.value) return '读取中'
  if (sending.value) return '回复中'
  return '就绪'
})
const stateModifier = computed(() => (sending.value ? 'busy' : sessionLoading.value ? 'idle' : 'ready'))

function createKey(role) {
  messageSeq += 1
  return `${role}-${messageSeq}`
}

function toolLabel(name) {
  return TOOL_LABELS[name] || name || '工具调用'
}

// 工具调用参数/结果是任意 JSON，这里一律按纯文本展示（模板插值会自动转义）。
function formatToolText(value) {
  if (value === null || value === undefined) return '（无）'
  if (typeof value === 'string') return value
  try {
    return JSON.stringify(value, null, 2)
  } catch {
    return String(value)
  }
}

// 带工具调用的一轮，后端把 assistant 正文存成
// {"text":"...","toolCalls":[...],"reasoningContent":"..."} 信封（历史接口原样返回，
// 流式 delta 则是纯文本）。这里统一解包，避免刷新后把 JSON 当正文渲染。
function unwrapAssistantContent(raw) {
  const text = String(raw ?? '')
  if (!text.startsWith('{')) return text
  try {
    const parsed = JSON.parse(text)
    if (parsed && typeof parsed.text === 'string') return parsed.text
  } catch {
    // 不是合法 JSON，按纯文本处理
  }
  return text
}

function buildHistory(list) {
  const mapped = []
  let lastAssistant = null
  for (const item of list || []) {
    if (!item || !item.role) continue
    if (item.role === 'user') {
      mapped.push({ key: createKey('user'), role: 'user', content: String(item.content ?? '') })
      lastAssistant = null
    } else if (item.role === 'assistant') {
      // 后端对「已落库但没有内容」的一轮会写占位文本，这里按空内容处理，
      // 显示成「已停止生成」而不是把这句占位当正文。
      const raw = String(item.content ?? '')
      const unwrapped = unwrapAssistantContent(raw)
      const content = unwrapped === '（本轮没有生成内容）' ? '' : unwrapped
      lastAssistant = {
        key: createKey('assistant'),
        role: 'assistant',
        id: item.id ?? null,
        content,
        status: 'done'
      }
      mapped.push(lastAssistant)
    } else if (item.role === 'tool') {
      const tool = {
        key: createKey('tool'),
        role: 'tool',
        toolName: item.toolName || '',
        args: item.toolArgs ?? null,
        result: typeof item.toolResult === 'string' ? item.toolResult : formatToolText(item.toolResult),
        status: 'done',
        ms: null
      }
      mapped.push(tool)
      // 历史里的工具消息归属它前面那条 assistant 正文，便于「停止」时回退。
      if (lastAssistant) lastAssistant.toolIndex = mapped.length - 1
    }
  }
  return mapped
}

async function loadSession() {
  sessionLoading.value = true
  sessionErrorText.value = ''
  try {
    const response = await getAgentSession(props.questionId)
    const data = response?.data || {}
    conversationId.value = data.conversationId ?? null
    messages.value = buildHistory(data.messages)
    lastUserText.value = [...messages.value].reverse().find((message) => message.role === 'user')?.content || ''
    await nextTick()
    forceScrollToBottom()
  } catch (error) {
    // 首次进入通常是 404（会话尚未创建）或后端未部署该接口，按空会话处理并给出提示。
    messages.value = []
    conversationId.value = null
    const status = error?.response?.status
    if (status === 404 || error?.response?.data?.code === 404) {
      sessionErrorText.value = '暂时读不到历史对话（后端 agent 接口尚未就绪），仍可直接提问。'
    } else if (!status) {
      sessionErrorText.value = '读取历史对话失败，请检查网络后重试。'
    } else {
      sessionErrorText.value = '读取历史对话失败，可直接开始新对话。'
    }
  } finally {
    sessionLoading.value = false
  }
}

function isNearBottom() {
  const element = scrollRef.value
  if (!element) return true
  return element.scrollHeight - element.scrollTop - element.clientHeight <= 96
}

function scrollToBottom(force = false) {
  const element = scrollRef.value
  if (!element) return
  if (!force && !isNearBottom()) return
  element.scrollTop = element.scrollHeight
}

function forceScrollToBottom() {
  scrollToBottom(true)
}

async function autoScroll() {
  const element = scrollRef.value
  if (!element || !isNearBottom()) return
  await nextTick()
  element.scrollTop = element.scrollHeight
}

function useExample(example) {
  draft.value = example
  send()
}

function onKeydown(event) {
  if (event.key !== 'Enter' || event.shiftKey || event.isComposing) return
  event.preventDefault()
  if (canSend.value) send()
}

function setAssistantText(message, text) {
  if (!message) return
  message.content = text
}

function handleEvent({ event, data }) {
  const payload = data && typeof data === 'object' ? data : {}
  if (event === 'meta') {
    if (payload.conversationId !== undefined && payload.conversationId !== null) {
      conversationId.value = payload.conversationId
    }
    if (payload.assistantMessageId !== undefined && payload.assistantMessageId !== null) {
      assistantMessageId.value = payload.assistantMessageId
      const message = currentAssistant.value
      if (message) message.id = payload.assistantMessageId
    }
    return
  }

  if (event === 'delta') {
    const message = currentAssistant.value
    if (message && typeof payload.text === 'string') {
      setAssistantText(message, message.content + payload.text)
      autoScroll()
    }
    return
  }

  if (event === 'tool') {
    const message = currentAssistant.value
    if (!message) return
    if (payload.status === 'running') {
      const tool = {
        key: createKey('tool'),
        role: 'tool',
        toolName: payload.name || '',
        args: payload.args ?? null,
        result: '',
        status: 'running',
        ms: null
      }
      messages.value.push(tool)
      message.toolIndex = messages.value.length - 1
    } else {
      const index = Number.isInteger(message.toolIndex) ? message.toolIndex : -1
      const tool = index >= 0 ? messages.value[index] : null
      if (tool) {
        tool.status = 'done'
        tool.result = formatToolText(payload.result)
        tool.ms = Number.isFinite(payload.ms) ? payload.ms : null
      } else {
        messages.value.push({
          key: createKey('tool'),
          role: 'tool',
          toolName: payload.name || '',
          args: null,
          result: formatToolText(payload.result),
          status: 'done',
          ms: Number.isFinite(payload.ms) ? payload.ms : null
        })
      }
    }
    autoScroll()
    return
  }

  if (event === 'done') {
    if (payload.assistantMessageId !== undefined && payload.assistantMessageId !== null) {
      assistantMessageId.value = payload.assistantMessageId
    }
    const message = currentAssistant.value
    if (message) {
      message.status = 'done'
      if (payload.assistantMessageId !== undefined && payload.assistantMessageId !== null) {
        message.id = payload.assistantMessageId
      }
      // done 之后服务端已落库，本地不再补写任何内容。
    }
    return
  }

  if (event === 'error') {
    const message = currentAssistant.value
    if (message) message.status = 'failed'
    const code = payload.code ? `（${payload.code}）` : ''
    errorText.value = `${payload.message || '生成失败，请重试。'}${code}`
  }
}

function stop() {
  if (!controller) return
  controller.abort()
}

async function send(overrides = {}) {
  if (sending.value) return
  const text = (overrides.text ?? draft.value).trim()
  if (!text) return

  draft.value = ''
  errorText.value = ''
  lastUserText.value = text
  assistantMessageId.value = null

  if (!overrides.skipUserMessage) {
    messages.value.push({ key: createKey('user'), role: 'user', content: text })
  }
  messages.value.push({
    key: createKey('assistant'),
    role: 'assistant',
    id: null,
    content: '',
    status: 'streaming',
    toolIndex: null
  })

  sending.value = true
  controller = new AbortController()
  let aborted = false
  forceScrollToBottom()

  try {
    await streamAgentChat(props.questionId, {
      message: text,
      conversationId: conversationId.value,
      signal: controller.signal,
      onEvent: handleEvent
    })
  } catch (error) {
    const message = currentAssistant.value
    if (error?.name === 'AbortError' || error?.code === 20) {
      aborted = true
    } else if (error?.code === 'unauthorized') {
      errorText.value = '登录已过期，请重新登录。'
      if (message) message.status = 'failed'
    } else {
      if (message) message.status = 'failed'
      errorText.value = error?.message || '生成失败，请重试。'
    }
  } finally {
    // 用户主动停止：保留已流式输出的文本，并把本轮标记为「已停止」。
    if (aborted) {
      const message = currentAssistant.value
      if (message) message.status = 'stopped'
    }
    sending.value = false
    controller = null
    await autoScroll()
    if (inputRef.value && typeof inputRef.value.focus === 'function') {
      inputRef.value.focus()
    }
  }
}

function retryLast() {
  if (sending.value || !lastUserText.value) return
  errorText.value = ''
  // 去掉本地这一轮失败的助手正文与工具卡片，再重发同一条问题。
  const tail = []
  for (let index = messages.value.length - 1; index >= 0; index -= 1) {
    if (messages.value[index].role === 'user') break
    tail.push(index)
  }
  for (const index of tail) messages.value.splice(index, 1)
  send({ text: lastUserText.value, skipUserMessage: true })
}

function dismissError() {
  errorText.value = ''
}

onMounted(() => {
  loadSession()
})

onBeforeUnmount(() => {
  if (controller) controller.abort()
})
</script>

<style scoped>
.agent-panel {
  display: flex;
  flex-direction: column;
  gap: var(--space-4);
  padding: var(--space-6);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-xl);
  background: var(--color-surface);
}

.agent-header {
  display: flex;
  align-items: flex-start;
  gap: var(--space-3);
}

.agent-icon {
  display: inline-flex;
  width: 40px;
  height: 40px;
  flex: 0 0 40px;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-md);
  color: var(--color-primary);
  background: var(--color-primary-light);
  font-size: var(--text-sm);
  font-weight: 700;
}

.agent-heading {
  flex: 1;
  min-width: 0;
}

.agent-heading h2 {
  font-size: var(--text-md);
}

.agent-heading p {
  margin-top: var(--space-1);
  color: var(--color-text-secondary);
  font-size: var(--text-sm);
}

.agent-state {
  flex: 0 0 auto;
  padding: 2px var(--space-2);
  border-radius: var(--radius-sm);
  color: var(--color-text-secondary);
  background: var(--color-surface-muted);
  font-size: var(--text-xs);
}

.agent-state.busy {
  color: var(--color-primary);
  background: var(--color-primary-light);
}

.agent-state.ready {
  color: #15803d;
  background: var(--color-success-light);
}

.agent-skeleton {
  padding: var(--space-2) 0;
}

.agent-stream {
  display: flex;
  height: clamp(240px, 34vh, 400px);
  flex-direction: column;
  gap: var(--space-3);
  overflow-y: auto;
  overflow-x: hidden;
  padding: var(--space-3);
  border: 1px solid var(--color-border-light);
  border-radius: var(--radius-lg);
  background: var(--color-surface-muted);
  scroll-behavior: smooth;
}

.agent-invite {
  margin: auto;
  padding: var(--space-4);
  text-align: center;
}

.agent-invite-title {
  color: var(--color-text-secondary);
  font-size: var(--text-sm);
}

.agent-chips {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-2);
  justify-content: center;
  margin-top: var(--space-3);
}

.agent-chip {
  min-height: 32px;
  padding: var(--space-2) var(--space-3);
  border: 1px solid var(--color-primary-border);
  border-radius: var(--radius-lg);
  color: var(--color-primary);
  background: var(--color-surface);
  cursor: pointer;
  font-size: var(--text-xs);
  transition: background var(--motion-fast) var(--ease-out), border-color var(--motion-fast) var(--ease-out);
}

.agent-chip:hover {
  border-color: var(--color-primary);
  background: var(--color-primary-light);
}

.agent-turn {
  display: grid;
  gap: var(--space-2);
}

.agent-row {
  display: grid;
  grid-template-columns: 30px minmax(0, 1fr);
  gap: var(--space-2);
  align-items: start;
}

.agent-avatar {
  display: inline-flex;
  width: 30px;
  height: 30px;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-sm);
  font-size: 11px;
  font-weight: 700;
}

.user-avatar {
  color: var(--color-text-secondary);
  background: var(--color-border-light);
}

.assistant-avatar {
  color: #fff;
  background: var(--color-primary);
}

.agent-bubble {
  min-width: 0;
  max-width: 100%;
  padding: var(--space-3);
  border-radius: var(--radius-md);
  font-size: var(--text-base);
  overflow-wrap: anywhere;
}

.user-bubble {
  border: 1px solid var(--color-primary-border);
  background: var(--color-primary-light);
}

.assistant-bubble {
  border: 1px solid var(--color-border);
  background: var(--color-surface);
}

.agent-plain {
  white-space: pre-wrap;
}

.agent-thinking {
  display: inline-flex;
  align-items: center;
  gap: var(--space-2);
  color: var(--color-text-secondary);
  font-size: var(--text-sm);
}

.agent-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: var(--color-primary);
  animation: agent-pulse 1.2s var(--ease-out) infinite;
}

@keyframes agent-pulse {
  0%,
  100% {
    opacity: 0.35;
  }

  50% {
    opacity: 1;
  }
}

.agent-note {
  display: inline-block;
  margin-top: var(--space-1);
  color: var(--color-text-secondary);
  font-size: var(--text-xs);
}

.failed-note {
  color: var(--color-danger);
}

.tool-row {
  grid-template-columns: minmax(0, 1fr);
  padding-left: 38px;
}

.agent-tool {
  min-width: 0;
  padding: var(--space-2) var(--space-3);
  border: 1px solid var(--color-border);
  border-left: 3px solid var(--color-primary);
  border-radius: var(--radius-md);
  background: var(--color-surface);
}

.agent-tool.running {
  border-left-color: var(--color-warning);
}

.agent-tool-head {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: var(--space-2);
}

.agent-tool-label {
  font-size: var(--text-sm);
  font-weight: 600;
}

.agent-tool-meta {
  color: var(--color-text-secondary);
  font-size: var(--text-xs);
}

.agent-tool-detail {
  margin-top: var(--space-2);
  color: var(--color-text-secondary);
  font-size: var(--text-xs);
}

.agent-tool-detail summary {
  cursor: pointer;
}

.agent-tool-caption {
  margin-top: var(--space-2);
  color: var(--color-text-secondary);
  font-size: var(--text-xs);
}

.agent-tool-pre {
  max-width: 100%;
  max-height: 180px;
  overflow: auto;
  margin-top: var(--space-1);
  padding: var(--space-2);
  border-radius: var(--radius-sm);
  background: var(--color-surface-muted);
  font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
  font-size: var(--text-xs);
  white-space: pre-wrap;
  overflow-wrap: anywhere;
}

.agent-error {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: var(--space-3);
  padding: var(--space-2) var(--space-3);
  border: 1px solid var(--color-danger);
  border-radius: var(--radius-md);
  color: #b91c1c;
  background: var(--color-danger-light);
  font-size: var(--text-sm);
}

.agent-error-text {
  flex: 1 1 200px;
  min-width: 0;
  overflow-wrap: anywhere;
}

.agent-error-actions {
  display: flex;
  gap: var(--space-2);
}

.agent-resume {
  display: flex;
  justify-content: flex-start;
}

.agent-composer {
  display: flex;
  flex-direction: column;
  gap: var(--space-2);
}

.agent-input {
  width: 100%;
  padding: var(--space-3);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-md);
  color: var(--color-text-primary);
  background: var(--color-surface);
  font: inherit;
  font-size: var(--text-base);
  resize: vertical;
}

.agent-input:focus-visible {
  border-color: var(--color-primary);
  outline: 3px solid #2563eb;
  outline-offset: 1px;
}

.agent-input:disabled {
  color: var(--color-text-tertiary);
  background: var(--color-surface-muted);
}

.agent-composer-foot {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: space-between;
  gap: var(--space-2);
}

.agent-hint {
  color: var(--color-text-secondary);
  font-size: var(--text-xs);
}

.agent-composer-actions {
  display: flex;
  gap: var(--space-2);
}

.agent-btn {
  min-height: 36px;
  padding: 0 var(--space-4);
  border: 1px solid var(--color-border-strong);
  border-radius: var(--radius-md);
  color: var(--color-text-primary);
  background: var(--color-surface);
  cursor: pointer;
  font-size: var(--text-sm);
  font-weight: 600;
  transition: background var(--motion-fast) var(--ease-out), border-color var(--motion-fast) var(--ease-out);
}

.agent-btn:hover:not(:disabled) {
  border-color: var(--color-primary);
  color: var(--color-primary);
  background: var(--color-primary-light);
}

.agent-btn:disabled {
  color: var(--color-text-tertiary);
  cursor: not-allowed;
  opacity: 0.7;
}

.agent-btn.primary {
  border-color: var(--color-primary);
  color: #fff;
  background: var(--color-primary);
}

.agent-btn.primary:hover:not(:disabled) {
  border-color: var(--color-primary-hover);
  color: #fff;
  background: var(--color-primary-hover);
}

.agent-btn.danger {
  border-color: var(--color-danger);
  color: var(--color-danger);
  background: var(--color-surface);
}

.agent-btn.ghost {
  border-color: var(--color-border);
}

@media (max-width: 640px) {
  .agent-panel {
    padding: 18px 16px;
  }

  .agent-icon {
    display: none;
  }

  .agent-state {
    order: 3;
  }

  .tool-row {
    padding-left: 0;
  }

  .agent-stream {
    height: clamp(240px, 52vh, 460px);
  }

  .agent-composer-foot {
    align-items: stretch;
    flex-direction: column;
  }

  .agent-composer-actions {
    justify-content: flex-end;
  }
}

@media (prefers-reduced-motion: reduce) {
  .agent-stream {
    scroll-behavior: auto;
  }

  .agent-dot {
    animation: none;
    opacity: 1;
  }
}
</style>
