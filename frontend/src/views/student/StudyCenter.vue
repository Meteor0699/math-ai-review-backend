<template>
  <div class="study-center">
    <div class="page-header">
      <div>
        <h2>学习记录</h2>
        <p>回看最近做过的题，集中整理错题本。</p>
      </div>
    </div>

    <el-tabs v-model="activeTab" @tab-change="handleTabChange">
      <el-tab-pane label="做题记录" name="records">
        <div v-loading="recordsLoading" class="question-stack">
          <el-empty v-if="!records.length && !recordsLoading" description="还没有做题记录" />
          <el-card
            v-for="item in records"
            :key="item.recordId"
            class="question-card"
            shadow="hover"
            @click="$router.push(`/questions/${item.questionId}`)"
          >
            <div class="card-main">
              <div>
                <div class="question-title">{{ item.title || '未命名题目' }}</div>
                <div class="question-meta">
                  <el-tag size="small" effect="plain">{{ item.courseName || '课程' }}</el-tag>
                  <el-tag size="small" type="info" effect="plain">{{ item.questionTypeLabel }}</el-tag>
                  <el-tag size="small" :type="statusTagType(item.status)">{{ statusLabel(item.status) }}</el-tag>
                  <el-tag v-if="item.isWrong" size="small" type="danger">错题</el-tag>
                </div>
                <div class="time-line">最近学习：{{ item.lastAttemptAt || '-' }} · 次数：{{ item.attemptCount || 1 }}</div>
              </div>
              <el-button text type="primary">查看</el-button>
            </div>
          </el-card>
        </div>
      </el-tab-pane>

      <el-tab-pane label="错题本" name="wrong">
        <div v-loading="wrongLoading" class="question-stack">
          <el-empty v-if="!wrongQuestions.length && !wrongLoading" description="错题本还是空的" />
          <el-card
            v-for="item in wrongQuestions"
            :key="item.wrongId"
            class="question-card"
            shadow="hover"
          >
            <div class="card-main">
              <div @click="$router.push(`/questions/${item.questionId}`)">
                <div class="question-title">{{ item.title || '未命名题目' }}</div>
                <div class="question-meta">
                  <el-tag size="small" effect="plain">{{ item.courseName || '课程' }}</el-tag>
                  <el-tag size="small" type="info" effect="plain">{{ item.questionTypeLabel }}</el-tag>
                  <el-tag size="small" type="danger">错题</el-tag>
                </div>
                <div class="time-line">加入时间：{{ item.addedAt || '-' }}</div>
              </div>
              <div class="card-actions">
                <el-button text type="primary" @click="$router.push(`/questions/${item.questionId}`)">复习</el-button>
                <el-button text type="danger" @click="removeFromWrong(item.questionId)">移出</el-button>
              </div>
            </div>
          </el-card>
        </div>
      </el-tab-pane>
    </el-tabs>
  </div>
</template>

<script setup>
import { onMounted, ref } from 'vue'
import { ElMessage } from 'element-plus'
import { getQuestionRecords, getWrongQuestions, removeWrongQuestion } from '../../api/course'

const activeTab = ref('records')
const records = ref([])
const wrongQuestions = ref([])
const recordsLoading = ref(false)
const wrongLoading = ref(false)

onMounted(async () => {
  await Promise.all([loadRecords(), loadWrongQuestions()])
})

function statusLabel(status) {
  if (status === 'mastered') return '已掌握'
  if (status === 'wrong') return '需复习'
  return '已学习'
}

function statusTagType(status) {
  if (status === 'mastered') return 'success'
  if (status === 'wrong') return 'danger'
  return 'info'
}

async function loadRecords() {
  recordsLoading.value = true
  try {
    const res = await getQuestionRecords({ page: 1, pageSize: 50 })
    records.value = res.data?.items || []
  } finally {
    recordsLoading.value = false
  }
}

async function loadWrongQuestions() {
  wrongLoading.value = true
  try {
    const res = await getWrongQuestions({ page: 1, pageSize: 50 })
    wrongQuestions.value = res.data?.items || []
  } finally {
    wrongLoading.value = false
  }
}

async function removeFromWrong(questionId) {
  await removeWrongQuestion(questionId)
  ElMessage.success('已移出错题本')
  await Promise.all([loadRecords(), loadWrongQuestions()])
}

async function handleTabChange(name) {
  if (name === 'records') {
    await loadRecords()
  } else {
    await loadWrongQuestions()
  }
}
</script>

<style scoped>
.study-center {
  max-width: 1040px;
  margin: 0 auto;
}

.page-header {
  display: flex;
  align-items: flex-end;
  justify-content: space-between;
  margin-bottom: 18px;
}

.page-header h2 {
  margin: 0 0 6px;
  font-size: 24px;
  color: #111827;
}

.page-header p {
  margin: 0;
  color: #6b7280;
}

.question-stack {
  display: flex;
  flex-direction: column;
  gap: 12px;
  min-height: 220px;
}

.question-card {
  border-radius: 8px;
  cursor: pointer;
}

.card-main {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.question-title {
  font-weight: 700;
  color: #111827;
  margin-bottom: 10px;
}

.question-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-bottom: 8px;
}

.time-line {
  color: #6b7280;
  font-size: 13px;
}

.card-actions {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-shrink: 0;
}
</style>
