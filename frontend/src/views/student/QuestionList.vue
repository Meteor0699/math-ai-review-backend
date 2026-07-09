<template>
  <div class="question-list">
    <div class="page-header">
      <div>
        <h2>题库练习</h2>
        <p>按题型和难度快速找到适合复习的题目</p>
      </div>
    </div>

    <div v-loading="loading" class="question-stack">
      <el-card
        v-for="q in list"
        :key="q.id"
        class="question-card"
        shadow="hover"
        :body-style="{ padding: '18px 20px' }"
        @click="$router.push(`/questions/${q.id}`)"
      >
        <div class="question-main">
          <div class="question-index">Q</div>
          <div class="question-content">
            <div class="question-title">{{ q.title || '未命名题目' }}</div>
            <div class="question-meta">
              <el-tag size="small" type="info" effect="plain">{{ typeLabel(q.questionType) }}</el-tag>
              <el-tag size="small" :type="difficultyType(q.difficulty)" effect="plain">
                {{ diffLabel(q.difficulty) }}
              </el-tag>
              <span v-if="q.courseName">{{ q.courseName }}</span>
              <span v-if="q.source">{{ q.source }}</span>
            </div>
          </div>
          <el-icon class="question-arrow"><ArrowRight /></el-icon>
        </div>
      </el-card>
      <el-empty v-if="!loading && list.length === 0" description="暂无题目" />
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { getQuestionList } from '../../api/course'

const route = useRoute()
const list = ref([])
const loading = ref(false)

const typeMap = {
  single_choice: '单选题',
  blank: '填空题',
  calculation: '计算题',
  solution: '解答题',
  proof: '证明题',
  judge: '判断题'
}
const diffMap = { basic: '基础', medium: '中等', hard: '较难' }

function typeLabel(val) {
  return typeMap[val] || val
}

function diffLabel(val) {
  return diffMap[val] || val
}

function difficultyType(val) {
  if (val === 'hard') return 'danger'
  if (val === 'medium') return 'warning'
  return 'success'
}

onMounted(async () => {
  loading.value = true
  try {
    const params = { page: 1, pageSize: 20 }
    if (route.query.chapterId) params.chapterId = route.query.chapterId
    if (route.query.courseId) params.courseId = route.query.courseId
    const res = await getQuestionList(params)
    list.value = res.data?.items || []
  } catch {
    list.value = []
  } finally {
    loading.value = false
  }
})
</script>

<style scoped>
.question-list {
  max-width: 940px;
  margin: 0 auto;
}

.page-header {
  margin-bottom: 18px;
}

.page-header h2 {
  font-size: 22px;
  color: #1f2937;
}

.page-header p {
  color: #909399;
  font-size: 14px;
  margin-top: 6px;
}

.question-stack {
  display: grid;
  gap: 12px;
}

.question-card {
  cursor: pointer;
  transition: transform 0.2s, border-color 0.2s;
}

.question-card:hover {
  transform: translateX(4px);
  border-color: #cfe0ff;
}

.question-main {
  display: flex;
  align-items: center;
  gap: 16px;
}

.question-index {
  width: 38px;
  height: 38px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 8px;
  background: #eaf2ff;
  color: #2563eb;
  font-weight: 700;
  flex-shrink: 0;
}

.question-content {
  flex: 1;
  min-width: 0;
}

.question-title {
  font-size: 16px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 9px;
}

.question-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
  font-size: 12px;
  color: #909399;
}

.question-arrow {
  color: #c0c4cc;
}
</style>
