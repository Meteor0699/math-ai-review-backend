<template>
  <div class="question-detail">
    <div class="page-header">
      <el-button text @click="$router.back()">
        <el-icon><ArrowLeft /></el-icon>
        返回
      </el-button>
      <h2>题目详情</h2>
    </div>

    <div v-loading="loading">
      <el-card v-if="question" class="detail-card">
        <div class="title-row">
          <h3>{{ question.title || '未命名题目' }}</h3>
          <el-tag v-if="studyState.isWrong" type="danger" effect="dark">错题本</el-tag>
        </div>

        <div class="meta">
          <el-tag>{{ question.questionTypeLabel || question.questionType }}</el-tag>
          <el-tag :type="difficultyTagType(question.difficulty)">
            {{ question.difficultyLabel || question.difficulty }}
          </el-tag>
          <el-tag v-if="studyState.status === 'mastered'" type="success">已掌握</el-tag>
          <el-tag v-else-if="studyState.status === 'viewed'" type="info">已学习</el-tag>
        </div>

        <div class="content" v-html="question.content"></div>
        <el-divider />

        <div class="answer-toggle">
          <div>
            <h4>{{ showAnswer ? '答案与解析' : '完成思考后再核对答案' }}</h4>
            <p v-if="!showAnswer">答案和普通解析默认隐藏，由你决定何时查看。</p>
          </div>
          <el-button
            :type="showAnswer ? 'default' : 'primary'"
            :plain="showAnswer"
            :aria-expanded="showAnswer"
            @click="showAnswer = !showAnswer"
          >
            <el-icon><Hide v-if="showAnswer" /><View v-else /></el-icon>
            {{ showAnswer ? '收起答案与解析' : '查看答案与解析' }}
          </el-button>
        </div>

        <el-collapse-transition>
          <div v-if="showAnswer" class="answer-panel">
            <h4>标准答案</h4>
            <div class="answer" v-html="question.standardAnswer"></div>
            <el-divider />

            <h4>普通解析</h4>
            <div class="explanation" v-html="question.normalExplanation || '暂无普通解析'"></div>
          </div>
        </el-collapse-transition>
        <el-divider />

        <div class="study-actions">
          <el-button type="primary" plain :loading="studyLoading" @click="markMastered">
            标记已掌握
          </el-button>
          <el-button
            v-if="!studyState.isWrong"
            type="danger"
            plain
            :loading="wrongLoading"
            @click="markWrong"
          >
            加入错题本
          </el-button>
          <el-button v-else plain :loading="wrongLoading" @click="unmarkWrong">
            移出错题本
          </el-button>
        </div>

        <el-divider />

        <div class="ai-toolbar">
          <el-button type="success" :loading="aiLoading" @click="requestAiExplanation">
            AI 题目讲解
          </el-button>
          <span v-if="aiCached !== null" class="cache-hint">
            {{ aiCached ? '已读取缓存解析' : '本次新生成解析，之后会直接复用' }}
          </span>
        </div>

        <section v-if="aiExplanation" class="ai-panel">
          <h4>AI 讲解</h4>
          <div class="rich-answer" v-html="renderedAiExplanation"></div>

          <div class="follow-up">
            <h4>继续追问</h4>
            <div v-if="followUps.length" class="follow-up-list">
              <div v-for="(item, index) in followUps" :key="index" class="follow-up-item">
                <div class="follow-up-question">{{ item.question }}</div>
                <div class="follow-up-answer rich-answer" v-html="renderMathContent(item.answer)"></div>
              </div>
            </div>
            <el-input
              v-model="followUpQuestion"
              type="textarea"
              :rows="3"
              maxlength="500"
              show-word-limit
              placeholder="围绕本题的步骤、公式、知识点继续追问"
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
        </section>
      </el-card>

      <el-empty v-if="!loading && !question" description="题目不存在" />
    </div>
  </div>
</template>

<script setup>
import { computed, ref, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { ElMessage } from 'element-plus'
import { ArrowLeft, Hide, View } from '@element-plus/icons-vue'
import {
  addWrongQuestion,
  askAiFollowUp,
  getAiExplanation,
  getQuestionDetail,
  getStudyState,
  recordQuestionStudy,
  removeWrongQuestion
} from '../../api/course'
import { renderMathContent } from '../../utils/renderMathContent'

const route = useRoute()
const questionId = route.params.questionId
const question = ref(null)
const aiExplanation = ref('')
const aiCached = ref(null)
const followUpQuestion = ref('')
const followUps = ref([])
const showAnswer = ref(false)
const studyState = ref({ status: 'none', isWrong: false })
const loading = ref(false)
const aiLoading = ref(false)
const followUpLoading = ref(false)
const studyLoading = ref(false)
const wrongLoading = ref(false)
const renderedAiExplanation = computed(() => renderMathContent(aiExplanation.value))

onMounted(async () => {
  loading.value = true
  try {
    const res = await getQuestionDetail(questionId)
    question.value = res.data
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

async function loadStudyState() {
  const res = await getStudyState(questionId)
  studyState.value = res.data || { status: 'none', isWrong: false }
}

async function markMastered() {
  studyLoading.value = true
  try {
    const res = await recordQuestionStudy(questionId, { status: 'mastered' })
    studyState.value = res.data
    ElMessage.success('已标记为掌握')
  } finally {
    studyLoading.value = false
  }
}

async function markWrong() {
  wrongLoading.value = true
  try {
    const res = await addWrongQuestion(questionId)
    studyState.value = res.data
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
    const res = await getAiExplanation(questionId)
    aiExplanation.value = res.data?.explanation || ''
    aiCached.value = Boolean(res.data?.cached)
    ElMessage.success(aiCached.value ? '已读取缓存 AI 讲解' : 'AI 讲解生成成功')
  } finally {
    aiLoading.value = false
  }
}

async function submitFollowUp() {
  const text = followUpQuestion.value.trim()
  if (!text) return

  followUpLoading.value = true
  try {
    const res = await askAiFollowUp(questionId, text)
    followUps.value.push({
      question: text,
      answer: res.data?.answer || ''
    })
    followUpQuestion.value = ''
  } finally {
    followUpLoading.value = false
  }
}
</script>

<style scoped>
.question-detail { max-width: 900px; margin: 0 auto; }
.page-header { display: flex; align-items: center; gap: 12px; margin-bottom: 20px; }
.page-header h2 { font-size: 22px; }
.title-row { display: flex; align-items: center; justify-content: space-between; gap: 12px; }
.detail-card h3 { font-size: 18px; margin-bottom: 12px; }
.meta { display: flex; flex-wrap: wrap; gap: 8px; margin-bottom: 16px; }
.content, .answer, .explanation, .ai-panel { font-size: 14px; line-height: 1.8; color: #303133; }
.answer-toggle {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  padding: 14px 16px;
  border: 1px solid #dbeafe;
  border-radius: 6px;
  background: #f8fbff;
}
.answer-toggle h4 { margin: 0; color: #1f2937; }
.answer-toggle p { margin: 5px 0 0; font-size: 13px; color: #6b7280; }
.answer-panel { padding: 18px 4px 2px; }
.answer-panel h4 { margin: 0 0 10px; }
.study-actions, .ai-toolbar { display: flex; align-items: center; flex-wrap: wrap; gap: 12px; }
.cache-hint { font-size: 13px; color: #6b7280; }
.ai-panel { margin-top: 16px; }
.follow-up { margin-top: 20px; padding-top: 16px; border-top: 1px solid #ebeef5; }
.follow-up-list { display: flex; flex-direction: column; gap: 12px; margin-bottom: 14px; }
.follow-up-item { border: 1px solid #ebeef5; border-radius: 6px; padding: 12px; background: #fafafa; }
.follow-up-question { font-weight: 600; margin-bottom: 8px; color: #303133; }
.follow-up-question::before { content: '问：'; color: #409eff; }
.follow-up-answer::before { content: '答：'; color: #67c23a; font-weight: 600; }
.follow-up-actions { display: flex; justify-content: flex-end; margin-top: 10px; }
.rich-answer :deep(p) { margin: 8px 0; }
.rich-answer :deep(.katex-display) { margin: 12px 0; overflow-x: auto; overflow-y: hidden; }
.rich-answer :deep(code) {
  padding: 2px 5px;
  border-radius: 4px;
  background: #f4f6f8;
  color: #606266;
}

@media (max-width: 640px) {
  .answer-toggle { align-items: stretch; flex-direction: column; }
  .answer-toggle .el-button { width: 100%; }
}
</style>
