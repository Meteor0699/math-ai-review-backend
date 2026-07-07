<template>
  <div class="question-list">
    <div class="page-header">
      <h2>题库</h2>
    </div>
    <div v-loading="loading">
      <el-card v-for="q in list" :key="q.id" class="question-card" style="margin-bottom: 12px; cursor: pointer"
        shadow="hover" @click="$router.push(`/questions/${q.id}`)">
        <div class="question-title">{{ q.title }}</div>
        <div class="question-meta">
          <el-tag size="small" type="info">{{ typeLabel(q.questionType) }}</el-tag>
          <el-tag size="small" :type="q.difficulty === 'hard' ? 'danger' : q.difficulty === 'medium' ? 'warning' : 'success'">
            {{ diffLabel(q.difficulty) }}
          </el-tag>
          <span v-if="q.source">{{ q.source }}</span>
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

const typeMap = { single_choice: '选择题', blank: '填空题', calculation: '计算题', proof: '证明题', judge: '判断题' }
const diffMap = { basic: '简单', medium: '中等', hard: '困难' }
function typeLabel(val) { return typeMap[val] || val }
function diffLabel(val) { return diffMap[val] || val }

onMounted(async () => {
  loading.value = true
  try {
    const params = { page: 1, pageSize: 20 }
    if (route.query.chapterId) params.chapterId = route.query.chapterId
    if (route.query.courseId) params.courseId = route.query.courseId
    const res = await getQuestionList(params)
    list.value = res.data?.items || []
  } catch (error) {
    // 错误已在 request 拦截器中处理
  } finally {
    loading.value = false
  }
})
</script>

<style scoped>
.question-list { max-width: 900px; margin: 0 auto; }
.page-header { margin-bottom: 20px; }
.page-header h2 { font-size: 22px; }
.question-title { font-size: 16px; font-weight: 600; margin-bottom: 8px; }
.question-meta { display: flex; gap: 8px; align-items: center; font-size: 12px; color: #909399; }
</style>