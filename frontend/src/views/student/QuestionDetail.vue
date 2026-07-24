<template>
  <div class="app-page question-detail">
    <PageHeader :title="question?.title || '题目详情'" description="先独立思考，再按需查看答案或请求 AI 分步讲解。">
      <template #breadcrumb>
        <el-breadcrumb separator="/">
          <el-breadcrumb-item :to="{ path: '/questions' }">题库练习</el-breadcrumb-item>
          <el-breadcrumb-item>题目详情</el-breadcrumb-item>
        </el-breadcrumb>
      </template>
      <template #actions>
        <el-button :icon="ArrowLeft" @click="$router.back()">返回</el-button>
      </template>
    </PageHeader>

    <div v-if="loading" class="detail-skeleton">
      <el-skeleton :rows="8" animated />
    </div>

    <div v-else-if="question" class="detail-layout">
      <main class="question-column">
        <section class="question-panel">
          <div class="question-meta">
            <el-tag>{{ question.questionTypeLabel || question.questionType }}</el-tag>
            <el-tag :type="difficultyTagType(question.difficulty)">
              {{ question.difficultyLabel || question.difficulty }}
            </el-tag>
            <el-tag v-if="question.source" type="info" effect="plain">{{ question.source }}</el-tag>
            <el-tag v-if="question.year" type="info" effect="plain">{{ question.year }}</el-tag>
            <el-tag v-if="studyState.isWrong" type="danger">错题本</el-tag>
          </div>

          <div class="question-content">
            <span class="content-label">题目</span>
            <FormulaContent :content="question.content" />
          </div>

          <div v-if="question.options?.length" class="question-options">
            <button
              v-for="option in question.options"
              :key="option.id"
              type="button"
              :class="{ correct: showAnswer && isCorrectOption(option.id) }"
            >
              <span>{{ option.optionLabel }}</span>
              <FormulaContent :content="option.optionContent" />
              <el-icon v-if="showAnswer && isCorrectOption(option.id)"><CircleCheckFilled /></el-icon>
            </button>
          </div>
        </section>

        <section class="answer-section">
          <div class="answer-heading">
            <div>
              <h2>{{ showAnswer ? '标准答案与普通解析' : '准备好核对答案了吗？' }}</h2>
              <p>{{ showAnswer ? '结合自己的思路，检查关键步骤和易错点。' : '答案不会随题目初始加载，点击后才会请求。' }}</p>
            </div>
            <el-button
              :type="showAnswer ? 'default' : 'primary'"
              :plain="showAnswer"
              :loading="answerLoading"
              :icon="showAnswer ? Hide : View"
              @click="toggleAnswer"
            >
              {{ showAnswer ? '收起答案' : '查看答案与解析' }}
            </el-button>
          </div>
          <el-collapse-transition>
            <div v-if="showAnswer" class="answer-content">
              <div>
                <h3><span>A</span>标准答案</h3>
                <FormulaContent :content="answerData?.standardAnswer || ''" />
              </div>
              <div>
                <h3><span>解</span>普通解析</h3>
                <FormulaContent :content="answerData?.normalExplanation || '暂无普通解析'" />
              </div>
            </div>
          </el-collapse-transition>
        </section>

        <section class="ai-section">
          <div class="ai-header">
            <span class="ai-icon"><el-icon><MagicStick /></el-icon></span>
            <div>
              <h2>AI 题目讲解</h2>
              <p>围绕本题考点、思路、步骤和易错点生成复习讲解。</p>
            </div>
            <el-button type="primary" :loading="aiLoading" @click="requestAiExplanation">
              {{ aiExplanation ? '重新读取讲解' : '生成 AI 讲解' }}
            </el-button>
          </div>

          <div v-if="aiCached !== null" class="cache-status">
            <el-icon><CircleCheck /></el-icon>
            {{ aiCached ? '已读取缓存讲解，没有重复调用模型' : '本次为新生成内容，后续将直接复用' }}
          </div>

          <div v-if="aiExplanation" class="ai-content">
            <FormulaContent :content="aiExplanation" />
            <div class="follow-up">
              <div class="follow-up-heading">
                <div>
                  <h3>继续追问</h3>
                  <p>只回答与当前题目或数学复习相关的问题。</p>
                </div>
              </div>
              <div v-if="followUps.length" class="follow-up-list">
                <div v-for="(item, index) in followUps" :key="index" class="follow-up-item">
                  <div class="student-question"><span>你</span>{{ item.question }}</div>
                  <div class="ai-answer"><span>AI</span><FormulaContent :content="item.answer" /></div>
                </div>
              </div>
              <el-input
                v-model="followUpQuestion"
                type="textarea"
                :rows="3"
                maxlength="500"
                show-word-limit
                placeholder="例如：为什么这里可以使用洛必达法则？"
              />
              <div class="follow-up-actions">
                <el-button
                  type="primary"
                  :disabled="!followUpQuestion.trim()"
                  :loading="followUpLoading"
                  @click="submitFollowUp"
                >
                  发送追问
                </el-button>
              </div>
            </div>
          </div>
          <div v-else class="ai-empty">
            <el-icon><ChatDotRound /></el-icon>
            <span>需要提示时再生成讲解，先给自己一点独立思考的时间。</span>
          </div>
        </section>
      </main>

      <aside class="study-panel">
        <h2>本题学习状态</h2>
        <div class="status-row">
          <span>当前状态</span>
          <el-tag :type="studyState.status === 'mastered' ? 'success' : 'info'">
            {{ studyState.status === 'mastered' ? '已掌握' : '已学习' }}
          </el-tag>
        </div>
        <div class="study-actions">
          <el-button type="primary" :loading="studyLoading" @click="markMastered">
            <el-icon><CircleCheck /></el-icon>标记已掌握
          </el-button>
          <el-button v-if="!studyState.isWrong" type="danger" plain :loading="wrongLoading" @click="markWrong">
            <el-icon><Warning /></el-icon>加入错题本
          </el-button>
          <el-button v-else plain :loading="wrongLoading" @click="unmarkWrong">移出错题本</el-button>
        </div>
        <div class="study-tip">
          <el-icon><InfoFilled /></el-icon>
          <p>标记掌握或加入错题本后，可在学习中心统一回看。</p>
        </div>
      </aside>
    </div>

    <el-empty v-else description="题目不存在或已下架" />
  </div>
</template>

<script setup>
import { onMounted, ref } from 'vue'
import { useRoute } from 'vue-router'
import { ArrowLeft, Hide, View } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import FormulaContent from '../../components/FormulaContent.vue'
import PageHeader from '../../components/PageHeader.vue'
import {
  addWrongQuestion,
  askAiFollowUp,
  getAiExplanation,
  getQuestionAnswer,
  getQuestionDetail,
  getStudyState,
  recordQuestionStudy,
  removeWrongQuestion
} from '../../api/course'

const route = useRoute()
const questionId = route.params.questionId
const question = ref(null)
const answerData = ref(null)
const aiExplanation = ref('')
const aiCached = ref(null)
const followUpQuestion = ref('')
const followUps = ref([])
const showAnswer = ref(false)
const studyState = ref({ status: 'none', isWrong: false })
const loading = ref(false)
const aiLoading = ref(false)
const answerLoading = ref(false)
const followUpLoading = ref(false)
const studyLoading = ref(false)
const wrongLoading = ref(false)

onMounted(async () => {
  loading.value = true
  try {
    const response = await getQuestionDetail(questionId)
    question.value = response.data
    await recordQuestionStudy(questionId, { status: 'viewed' })
    await loadStudyState()
  } finally {
    loading.value = false
  }
})

function difficultyTagType(difficulty) {
  if (difficulty === 'hard' || difficulty === '较难') return 'danger'
  if (difficulty === 'medium' || difficulty === '中等') return 'warning'
  return 'success'
}

function isCorrectOption(optionId) {
  return answerData.value?.options?.some((option) => Number(option.id) === Number(optionId) && Boolean(option.isCorrect))
}

async function toggleAnswer() {
  if (showAnswer.value) {
    showAnswer.value = false
    return
  }
  if (!answerData.value) {
    answerLoading.value = true
    try {
      const response = await getQuestionAnswer(questionId)
      answerData.value = response.data
    } finally {
      answerLoading.value = false
    }
  }
  showAnswer.value = true
}

async function loadStudyState() {
  const response = await getStudyState(questionId)
  studyState.value = response.data || { status: 'none', isWrong: false }
}

async function markMastered() {
  studyLoading.value = true
  try {
    const response = await recordQuestionStudy(questionId, { status: 'mastered' })
    studyState.value = response.data
    ElMessage.success('已标记为掌握')
  } finally {
    studyLoading.value = false
  }
}

async function markWrong() {
  wrongLoading.value = true
  try {
    const response = await addWrongQuestion(questionId)
    studyState.value = response.data
    ElMessage.success('已加入错题本')
  } finally {
    wrongLoading.value = false
  }
}

async function unmarkWrong() {
  wrongLoading.value = true
  try {
    await removeWrongQuestion(questionId)
    await loadStudyState()
    ElMessage.success('已移出错题本')
  } finally {
    wrongLoading.value = false
  }
}

async function requestAiExplanation() {
  aiLoading.value = true
  try {
    const response = await getAiExplanation(questionId)
    aiExplanation.value = response.data?.explanation || ''
    aiCached.value = Boolean(response.data?.cached)
    ElMessage.success(aiCached.value ? '已读取缓存讲解' : 'AI 讲解生成成功')
  } finally {
    aiLoading.value = false
  }
}

async function submitFollowUp() {
  const text = followUpQuestion.value.trim()
  if (!text) return
  followUpLoading.value = true
  try {
    const response = await askAiFollowUp(questionId, text)
    followUps.value.push({ question: text, answer: response.data?.answer || '' })
    followUpQuestion.value = ''
  } finally {
    followUpLoading.value = false
  }
}
</script>

<style scoped>
.detail-skeleton,
.question-panel,
.answer-section,
.ai-section,
.study-panel {
  padding: 24px;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-xl);
  background: var(--color-surface);
}

.detail-layout {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 280px;
  align-items: start;
  gap: 20px;
}

.question-column {
  display: grid;
  gap: 18px;
}

.question-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-bottom: 24px;
}

.question-content {
  min-height: 120px;
  font-size: 16px;
}

.content-label {
  display: block;
  margin-bottom: 10px;
  color: var(--color-text-secondary);
  font-size: var(--text-xs);
  font-weight: 600;
}

.question-options {
  display: grid;
  gap: 10px;
  margin-top: 24px;
}

.question-options button {
  display: grid;
  grid-template-columns: 32px minmax(0, 1fr) 24px;
  align-items: center;
  gap: 10px;
  min-height: 52px;
  padding: 10px 14px;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-md);
  text-align: left;
  background: var(--color-surface);
}

.question-options button > span {
  display: inline-flex;
  width: 30px;
  height: 30px;
  align-items: center;
  justify-content: center;
  border-radius: 50%;
  color: var(--color-primary);
  background: var(--color-primary-light);
  font-weight: 700;
}

.question-options button.correct {
  border-color: var(--color-success);
  background: var(--color-success-light);
}

.question-options button.correct > .el-icon {
  color: var(--color-success);
}

.answer-heading,
.ai-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.answer-heading h2,
.ai-header h2,
.study-panel h2 {
  font-size: var(--text-md);
}

.answer-heading p,
.ai-header p {
  margin-top: 4px;
  color: var(--color-text-secondary);
  font-size: var(--text-sm);
}

.answer-content {
  display: grid;
  gap: 22px;
  margin-top: 22px;
  padding-top: 22px;
  border-top: 1px solid var(--color-border);
}

.answer-content h3 {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 10px;
  font-size: var(--text-base);
}

.answer-content h3 span {
  display: inline-flex;
  width: 28px;
  height: 28px;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-sm);
  color: var(--color-primary);
  background: var(--color-primary-light);
}

.ai-section {
  border-color: #bfdbfe;
}

.ai-header {
  justify-content: flex-start;
}

.ai-header > div {
  flex: 1;
}

.ai-icon {
  display: inline-flex;
  width: 44px;
  height: 44px;
  flex: 0 0 44px;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-md);
  color: var(--color-primary);
  background: var(--color-primary-light);
  font-size: 22px;
}

.cache-status {
  display: flex;
  align-items: center;
  gap: 7px;
  margin-top: 16px;
  padding: 8px 10px;
  border-radius: var(--radius-md);
  color: #15803d;
  background: var(--color-success-light);
  font-size: var(--text-xs);
}

.ai-content {
  margin-top: 22px;
  padding-top: 22px;
  border-top: 1px solid var(--color-border);
}

.ai-empty {
  display: flex;
  min-height: 120px;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  gap: 10px;
  margin-top: 20px;
  border-radius: var(--radius-lg);
  color: var(--color-text-secondary);
  background: var(--color-surface-muted);
  text-align: center;
}

.ai-empty .el-icon {
  color: var(--color-primary);
  font-size: 28px;
}

.follow-up {
  margin-top: 24px;
  padding-top: 22px;
  border-top: 1px solid var(--color-border);
}

.follow-up-heading {
  margin-bottom: 14px;
}

.follow-up-heading h3 {
  font-size: var(--text-base);
}

.follow-up-heading p {
  color: var(--color-text-secondary);
  font-size: var(--text-xs);
}

.follow-up-list {
  display: grid;
  gap: 12px;
  margin-bottom: 14px;
}

.follow-up-item {
  display: grid;
  gap: 10px;
}

.student-question,
.ai-answer {
  display: grid;
  grid-template-columns: 32px minmax(0, 1fr);
  gap: 8px;
  align-items: start;
}

.student-question > span,
.ai-answer > span {
  display: inline-flex;
  width: 30px;
  height: 30px;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-sm);
  color: #fff;
  background: var(--color-primary);
  font-size: 11px;
}

.ai-answer {
  padding: 12px;
  border-radius: var(--radius-md);
  background: var(--color-surface-muted);
}

.ai-answer > span {
  color: var(--color-primary);
  background: var(--color-primary-light);
}

.follow-up-actions {
  display: flex;
  justify-content: flex-end;
  margin-top: 10px;
}

.study-panel {
  position: sticky;
  top: calc(var(--header-height) + 20px);
}

.status-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin: 18px 0;
  padding: 12px 0;
  border-top: 1px solid var(--color-border);
  border-bottom: 1px solid var(--color-border);
  color: var(--color-text-secondary);
  font-size: var(--text-sm);
}

.study-actions {
  display: grid;
  gap: 10px;
}

.study-actions .el-button {
  width: 100%;
  margin: 0;
}

.study-tip {
  display: flex;
  align-items: flex-start;
  gap: 8px;
  margin-top: 18px;
  padding: 12px;
  border-radius: var(--radius-md);
  color: var(--color-text-secondary);
  background: var(--color-surface-muted);
  font-size: var(--text-xs);
}

.study-tip .el-icon {
  margin-top: 3px;
  color: var(--color-primary);
}

@media (max-width: 980px) {
  .detail-layout {
    grid-template-columns: 1fr;
  }

  .study-panel {
    position: static;
  }

  .study-actions {
    grid-template-columns: 1fr 1fr;
  }
}

@media (max-width: 640px) {
  .detail-skeleton,
  .question-panel,
  .answer-section,
  .ai-section,
  .study-panel {
    padding: 18px 16px;
  }

  .answer-heading,
  .ai-header {
    align-items: stretch;
    flex-direction: column;
  }

  .ai-icon {
    display: none;
  }

  .answer-heading .el-button,
  .ai-header .el-button {
    width: 100%;
  }

  .study-actions {
    grid-template-columns: 1fr;
  }
}
</style>
