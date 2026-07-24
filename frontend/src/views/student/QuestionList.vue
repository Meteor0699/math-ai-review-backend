<template>
  <div class="app-page question-page">
    <PageHeader title="题库练习" description="按课程、章节、题型和难度找到当前需要的题目。" />

    <section class="filter-panel">
      <el-input v-model="filters.keyword" :prefix-icon="Search" clearable placeholder="搜索题目标题或题干" @keyup.enter="applyFilters" />
      <el-select v-model="filters.courseId" clearable placeholder="全部课程" @change="handleCourseChange">
        <el-option v-for="course in courses" :key="course.id" :label="course.courseName" :value="course.id" />
      </el-select>
      <el-select v-model="filters.chapterId" clearable placeholder="全部章节" :disabled="!filters.courseId">
        <el-option v-for="chapter in chapters" :key="chapter.id" :label="chapter.chapterName || chapter.title" :value="chapter.id" />
      </el-select>
      <el-select v-model="filters.questionType" clearable placeholder="全部题型">
        <el-option v-for="item in questionTypes" :key="item.value" :label="item.label" :value="item.value" />
      </el-select>
      <el-select v-model="filters.difficulty" clearable placeholder="全部难度">
        <el-option label="基础" value="basic" />
        <el-option label="中等" value="medium" />
        <el-option label="较难" value="hard" />
      </el-select>
      <el-input v-model="filters.source" clearable placeholder="题目来源" />
      <div class="filter-actions">
        <el-button type="primary" :icon="Search" @click="applyFilters">筛选</el-button>
        <el-button :icon="Refresh" @click="resetFilters">重置</el-button>
      </div>
    </section>

    <div class="result-header">
      <span>共 {{ total }} 道题目</span>
      <small>答案与解析将在你主动查看后加载</small>
    </div>

    <div v-if="loading" class="skeleton-list">
      <el-skeleton v-for="index in 5" :key="index" :rows="2" animated />
    </div>

    <div v-else class="question-list">
      <article v-for="(question, index) in questions" :key="question.id" class="question-card" @click="$router.push(`/questions/${question.id}`)">
        <span class="question-number">{{ String((page - 1) * pageSize + index + 1).padStart(2, '0') }}</span>
        <div class="question-copy">
          <h2>{{ question.title || '未命名题目' }}</h2>
          <p>{{ plainText(question.content) }}</p>
          <div class="question-meta">
            <el-tag size="small" effect="plain">{{ question.questionTypeLabel || typeLabel(question.questionType) }}</el-tag>
            <el-tag size="small" :type="difficultyType(question.difficulty)" effect="plain">
              {{ question.difficultyLabel || difficultyLabel(question.difficulty) }}
            </el-tag>
            <span v-if="question.courseName"><el-icon><Collection /></el-icon>{{ question.courseName }}</span>
            <span v-if="question.source"><el-icon><Tickets /></el-icon>{{ question.source }}</span>
            <span v-if="question.year"><el-icon><Calendar /></el-icon>{{ question.year }}</span>
          </div>
        </div>
        <el-button type="primary" plain>查看题目</el-button>
      </article>
      <el-empty v-if="!questions.length" description="没有找到符合条件的题目，请调整筛选条件" />
    </div>

    <el-pagination
      v-if="total > pageSize"
      v-model:current-page="page"
      :page-size="pageSize"
      :total="total"
      layout="prev, pager, next"
      class="pagination"
      @current-change="loadQuestions"
    />
  </div>
</template>

<script setup>
import { onMounted, reactive, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { Refresh, Search } from '@element-plus/icons-vue'
import PageHeader from '../../components/PageHeader.vue'
import { getChapters, getCourseList, getQuestionList } from '../../api/course'

const route = useRoute()
const router = useRouter()
const courses = ref([])
const chapters = ref([])
const questions = ref([])
const loading = ref(false)
const total = ref(0)
const page = ref(1)
const pageSize = 10
const filters = reactive({
  keyword: '',
  courseId: '',
  chapterId: '',
  questionType: '',
  difficulty: '',
  source: ''
})

const questionTypes = [
  { label: '单选题', value: 'single_choice' },
  { label: '填空题', value: 'blank' },
  { label: '计算题', value: 'calculation' },
  { label: '解答题', value: 'solution' },
  { label: '证明题', value: 'proof' }
]

onMounted(async () => {
  const response = await getCourseList({ page: 1, pageSize: 100 }).catch(() => null)
  courses.value = response?.data?.items || []
  syncRouteFilters()
  if (filters.courseId) await loadChapters()
  await loadQuestions()
})

watch(() => route.query, async () => {
  syncRouteFilters()
  if (filters.courseId) await loadChapters()
  await loadQuestions()
})

function syncRouteFilters() {
  filters.keyword = route.query.keyword || ''
  filters.courseId = route.query.courseId ? Number(route.query.courseId) : ''
  filters.chapterId = route.query.chapterId ? Number(route.query.chapterId) : ''
}

async function loadChapters() {
  const response = await getChapters(filters.courseId, { page: 1, pageSize: 100 }).catch(() => null)
  chapters.value = response?.data?.items || []
}

async function handleCourseChange() {
  filters.chapterId = ''
  chapters.value = []
  if (filters.courseId) await loadChapters()
}

async function loadQuestions() {
  loading.value = true
  try {
    const params = { page: page.value, pageSize }
    Object.entries(filters).forEach(([key, value]) => {
      if (value !== '' && value !== null) params[key] = value
    })
    const response = await getQuestionList(params)
    questions.value = response.data?.items || []
    total.value = Number(response.data?.total || questions.value.length)
  } finally {
    loading.value = false
  }
}

async function applyFilters() {
  page.value = 1
  const query = {}
  Object.entries(filters).forEach(([key, value]) => {
    if (value !== '' && value !== null) query[key] = value
  })
  await router.replace({ path: '/questions', query })
}

async function resetFilters() {
  Object.assign(filters, { keyword: '', courseId: '', chapterId: '', questionType: '', difficulty: '', source: '' })
  chapters.value = []
  page.value = 1
  await router.replace('/questions')
}

function plainText(value) {
  return String(value || '').replace(/<[^>]*>/g, '').replace(/\\[()[\]]/g, '').slice(0, 180)
}

function typeLabel(value) {
  return questionTypes.find((item) => item.value === value)?.label || value
}

function difficultyLabel(value) {
  return { basic: '基础', medium: '中等', hard: '较难' }[value] || value
}

function difficultyType(value) {
  return { basic: 'success', medium: 'warning', hard: 'danger' }[value] || 'info'
}
</script>

<style scoped>
.filter-panel {
  display: grid;
  grid-template-columns: minmax(220px, 1.4fr) repeat(5, minmax(130px, 0.7fr)) auto;
  gap: 10px;
  padding: 16px;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  background: var(--color-surface);
}

.filter-actions {
  display: flex;
  gap: 8px;
}

.result-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin: 20px 2px 12px;
  color: var(--color-text-secondary);
}

.result-header small {
  font-size: var(--text-xs);
}

.skeleton-list,
.question-list {
  display: grid;
  gap: 12px;
}

.skeleton-list > * {
  padding: 20px;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  background: var(--color-surface);
}

.question-card {
  display: grid;
  grid-template-columns: 48px minmax(0, 1fr) auto;
  align-items: center;
  gap: 16px;
  min-height: 126px;
  padding: 18px 20px;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  background: var(--color-surface);
  cursor: pointer;
  transition: border-color var(--motion-fast), transform var(--motion-fast) var(--ease-out);
}

.question-card:hover {
  transform: translateY(-2px);
  border-color: var(--color-primary);
}

.question-number {
  display: inline-flex;
  width: 42px;
  height: 42px;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-md);
  color: var(--color-primary);
  background: var(--color-primary-light);
  font-weight: 700;
}

.question-copy {
  min-width: 0;
}

.question-copy h2 {
  font-size: var(--text-md);
}

.question-copy p {
  display: -webkit-box;
  overflow: hidden;
  margin: 6px 0 10px;
  color: var(--color-text-secondary);
  font-size: var(--text-sm);
  -webkit-box-orient: vertical;
  -webkit-line-clamp: 2;
}

.question-meta {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px 12px;
  color: var(--color-text-secondary);
  font-size: var(--text-xs);
}

.question-meta > span {
  display: inline-flex;
  align-items: center;
  gap: 4px;
}

.pagination {
  justify-content: center;
  margin-top: 24px;
}

@media (max-width: 1200px) {
  .filter-panel {
    grid-template-columns: repeat(3, minmax(160px, 1fr));
  }
}

@media (max-width: 700px) {
  .filter-panel {
    grid-template-columns: 1fr 1fr;
  }

  .filter-panel > :first-child,
  .filter-actions {
    grid-column: 1 / -1;
  }

  .filter-actions .el-button {
    flex: 1;
  }

  .result-header small {
    display: none;
  }

  .question-card {
    grid-template-columns: 42px minmax(0, 1fr);
    padding: 16px;
  }

  .question-card > .el-button {
    display: none;
  }
}
</style>
