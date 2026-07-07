<template>
  <div class="question-detail">
    <div class="page-header">
      <el-button text @click="$router.back()">
        <el-icon><ArrowLeft /></el-icon> 返回
      </el-button>
      <h2>题目详情</h2>
    </div>

    <div v-loading="loading">
      <el-card v-if="question" class="detail-card">
        <h3>{{ question.title }}</h3>
        <div class="meta">
          <el-tag>{{ question.questionType }}</el-tag>
          <el-tag :type="difficultyTagType(question.difficulty)">
            {{ question.difficulty }}
          </el-tag>
        </div>

        <div class="content" v-html="question.content"></div>
        <el-divider />

        <h4>标准答案</h4>
        <div class="answer" v-html="question.standardAnswer"></div>
        <el-divider />

        <h4>普通解析</h4>
        <div class="explanation" v-html="question.normalExplanation"></div>
        <el-divider />

        <div class="ai-toolbar">
          <el-button type="success" :loading="aiLoading" @click="requestAiExplanation">
            AI 题目讲解
          </el-button>
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
import { ArrowLeft } from '@element-plus/icons-vue'
import { getQuestionDetail, getAiExplanation, askAiFollowUp } from '../../api/course'
import { renderMathContent } from '../../utils/renderMathContent'

const route = useRoute()
const questionId = route.params.questionId
const question = ref(null)
const aiExplanation = ref('')
const followUpQuestion = ref('')
const followUps = ref([])
const loading = ref(false)
const aiLoading = ref(false)
const followUpLoading = ref(false)
const renderedAiExplanation = computed(() => renderMathContent(aiExplanation.value))

onMounted(async () => {
  loading.value = true
  try {
    const res = await getQuestionDetail(questionId)
    question.value = res.data
  } finally {
    loading.value = false
  }
})

function difficultyTagType(difficulty) {
  if (difficulty === 'hard' || difficulty === '较难') return 'danger'
  if (difficulty === 'medium' || difficulty === '中等') return 'warning'
  return 'success'
}

async function requestAiExplanation() {
  aiLoading.value = true
  try {
    const res = await getAiExplanation(questionId)
    aiExplanation.value = res.data?.explanation || ''
    ElMessage.success(res.data?.cached ? '已加载 AI 讲解' : 'AI 讲解生成成功')
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
.detail-card h3 { font-size: 18px; margin-bottom: 12px; }
.meta { display: flex; gap: 8px; margin-bottom: 16px; }
.content, .answer, .explanation, .ai-panel { font-size: 14px; line-height: 1.8; color: #303133; }
.ai-toolbar { display: flex; align-items: center; gap: 12px; }
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
</style>
