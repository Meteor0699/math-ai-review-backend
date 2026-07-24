<template>
  <div class="app-page study-center">
    <PageHeader title="学习中心" description="回看学习记录，集中复习仍未掌握的错题。" />

    <section class="study-panel">
      <el-tabs v-model="activeTab" @tab-change="handleTabChange">
        <el-tab-pane name="records">
          <template #label><span class="tab-label"><el-icon><Notebook /></el-icon>学习记录</span></template>
        </el-tab-pane>
        <el-tab-pane name="wrong">
          <template #label>
            <span class="tab-label"><el-icon><Warning /></el-icon>错题本 <el-badge :value="wrongQuestions.length" :hidden="!wrongQuestions.length" /></span>
          </template>
        </el-tab-pane>
      </el-tabs>

      <div class="study-filters">
        <el-input v-model="keyword" :prefix-icon="Search" clearable placeholder="搜索题目名称" />
        <el-select v-model="courseFilter" clearable placeholder="全部课程">
          <el-option v-for="course in availableCourses" :key="course" :label="course" :value="course" />
        </el-select>
        <el-select v-model="typeFilter" clearable placeholder="全部题型">
          <el-option v-for="type in availableTypes" :key="type" :label="type" :value="type" />
        </el-select>
      </div>

      <div v-loading="currentLoading" class="study-list">
        <article v-for="item in pagedItems" :key="item.recordId || item.wrongId" class="study-row">
          <span class="row-icon" :class="{ wrong: activeTab === 'wrong' }">
            <el-icon><Warning v-if="activeTab === 'wrong'" /><EditPen v-else /></el-icon>
          </span>
          <div class="row-main">
            <h2>{{ item.title || '未命名题目' }}</h2>
            <div class="row-meta">
              <el-tag size="small" effect="plain">{{ item.courseName || '课程' }}</el-tag>
              <el-tag size="small" type="info" effect="plain">{{ item.questionTypeLabel || '题目' }}</el-tag>
              <el-tag v-if="activeTab === 'records'" size="small" :type="statusTagType(item.status)">
                {{ statusLabel(item.status) }}
              </el-tag>
              <el-tag v-else size="small" type="danger">待复习</el-tag>
            </div>
            <p v-if="activeTab === 'records'">
              最近学习 {{ item.lastAttemptAt || '-' }} · 累计 {{ item.attemptCount || 1 }} 次
            </p>
            <p v-else>加入错题本 {{ item.addedAt || '-' }}</p>
          </div>
          <div class="row-actions">
            <el-button type="primary" plain @click="$router.push(`/questions/${item.questionId}`)">
              {{ activeTab === 'wrong' ? '开始复习' : '查看题目' }}
            </el-button>
            <el-button v-if="activeTab === 'wrong'" type="danger" text @click="removeFromWrong(item.questionId)">移出</el-button>
          </div>
        </article>
        <el-empty
          v-if="!currentLoading && !filteredItems.length"
          :description="activeTab === 'wrong' ? '错题本还是空的，遇到需要加强的题目时可以加入这里' : '完成题目学习后，记录会出现在这里'"
        />
      </div>

      <el-pagination
        v-if="filteredItems.length > pageSize"
        v-model:current-page="page"
        :page-size="pageSize"
        :total="filteredItems.length"
        layout="prev, pager, next"
        class="pagination"
      />
    </section>
  </div>
</template>

<script setup>
import { computed, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { Search } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import PageHeader from '../../components/PageHeader.vue'
import { getQuestionRecords, getWrongQuestions, removeWrongQuestion } from '../../api/course'

const route = useRoute()
const router = useRouter()
const activeTab = ref(route.query.tab === 'wrong' ? 'wrong' : 'records')
const records = ref([])
const wrongQuestions = ref([])
const recordsLoading = ref(false)
const wrongLoading = ref(false)
const keyword = ref('')
const courseFilter = ref('')
const typeFilter = ref('')
const page = ref(1)
const pageSize = 10

const sourceItems = computed(() => activeTab.value === 'records' ? records.value : wrongQuestions.value)
const currentLoading = computed(() => activeTab.value === 'records' ? recordsLoading.value : wrongLoading.value)
const availableCourses = computed(() => [...new Set(sourceItems.value.map((item) => item.courseName).filter(Boolean))])
const availableTypes = computed(() => [...new Set(sourceItems.value.map((item) => item.questionTypeLabel).filter(Boolean))])
const filteredItems = computed(() => sourceItems.value.filter((item) => {
  const matchesKeyword = !keyword.value || (item.title || '').toLowerCase().includes(keyword.value.toLowerCase())
  const matchesCourse = !courseFilter.value || item.courseName === courseFilter.value
  const matchesType = !typeFilter.value || item.questionTypeLabel === typeFilter.value
  return matchesKeyword && matchesCourse && matchesType
}))
const pagedItems = computed(() => {
  const start = (page.value - 1) * pageSize
  return filteredItems.value.slice(start, start + pageSize)
})

watch([keyword, courseFilter, typeFilter], () => {
  page.value = 1
})

onMounted(async () => {
  await Promise.all([loadRecords(), loadWrongQuestions()])
})

function statusLabel(status) {
  return { mastered: '已掌握', wrong: '需复习', viewed: '已学习' }[status] || '已学习'
}

function statusTagType(status) {
  return { mastered: 'success', wrong: 'danger', viewed: 'info' }[status] || 'info'
}

async function loadRecords() {
  recordsLoading.value = true
  try {
    const response = await getQuestionRecords({ page: 1, pageSize: 100 })
    records.value = response.data?.items || []
  } finally {
    recordsLoading.value = false
  }
}

async function loadWrongQuestions() {
  wrongLoading.value = true
  try {
    const response = await getWrongQuestions({ page: 1, pageSize: 100 })
    wrongQuestions.value = response.data?.items || []
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
  keyword.value = ''
  courseFilter.value = ''
  typeFilter.value = ''
  page.value = 1
  await router.replace({ path: '/study', query: name === 'wrong' ? { tab: 'wrong' } : {} })
}
</script>

<style scoped>
.study-panel {
  padding: 0 22px 22px;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-xl);
  background: var(--color-surface);
}

.tab-label {
  display: inline-flex;
  min-height: 44px;
  align-items: center;
  gap: 7px;
}

.study-filters {
  display: grid;
  grid-template-columns: minmax(240px, 1fr) 180px 180px;
  gap: 10px;
  padding: 6px 0 18px;
}

.study-list {
  display: grid;
  gap: 10px;
  min-height: 260px;
}

.study-row {
  display: grid;
  grid-template-columns: 44px minmax(0, 1fr) auto;
  align-items: center;
  gap: 14px;
  min-height: 104px;
  padding: 14px 16px;
  border-radius: var(--radius-lg);
  background: var(--color-surface-muted);
}

.row-icon {
  display: inline-flex;
  width: 42px;
  height: 42px;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-md);
  color: var(--color-primary);
  background: var(--color-primary-light);
}

.row-icon.wrong {
  color: #dc2626;
  background: var(--color-danger-light);
}

.row-main {
  min-width: 0;
}

.row-main h2 {
  overflow: hidden;
  font-size: var(--text-base);
  text-overflow: ellipsis;
  white-space: nowrap;
}

.row-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 7px;
  margin: 7px 0;
}

.row-main p {
  color: var(--color-text-secondary);
  font-size: var(--text-xs);
}

.row-actions {
  display: flex;
  align-items: center;
  gap: 6px;
}

.row-actions .el-button {
  margin: 0;
}

.pagination {
  justify-content: center;
  margin-top: 22px;
}

@media (max-width: 760px) {
  .study-panel {
    padding: 0 14px 16px;
  }

  .study-filters {
    grid-template-columns: 1fr 1fr;
  }

  .study-filters > :first-child {
    grid-column: 1 / -1;
  }

  .study-row {
    grid-template-columns: 40px minmax(0, 1fr);
  }

  .row-actions {
    grid-column: 1 / -1;
    justify-content: flex-end;
  }
}
</style>
