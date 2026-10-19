<template>
  <div class="app-page home-page">
    <section class="welcome-panel">
      <div class="welcome-copy">
        <span>{{ greeting }}，{{ userStore.realName || userStore.username || '同学' }}</span>
        <h1>今天从一个知识点开始</h1>
        <p>先复习章节重点，再进入题库验证理解。遇到卡点时，让 AI 按步骤讲清楚。</p>
        <div class="welcome-actions">
          <el-button type="primary" @click="router.push('/courses')">继续课程学习</el-button>
          <el-button @click="router.push('/questions')">进入题库</el-button>
        </div>
      </div>
      <div class="study-route" aria-label="推荐复习顺序">
        <span>推荐复习顺序</span>
        <ol>
          <li><strong>1</strong>看重点</li>
          <li><strong>2</strong>做题目</li>
          <li><strong>3</strong>查解析</li>
        </ol>
      </div>
    </section>

    <section class="stats-grid" aria-label="学习概览">
      <StatCard label="学习记录" :value="records.length" hint="已产生的题目学习记录" icon="Notebook" />
      <StatCard label="已掌握" :value="masteredCount" hint="标记为已掌握的题目" icon="CircleCheck" tone="success" />
      <StatCard label="错题本" :value="wrongQuestions.length" hint="仍需继续复习的题目" icon="Warning" tone="danger" />
      <StatCard label="当前课程" :value="courses.length" hint="平台已开放课程" icon="Collection" tone="warning" />
    </section>

    <div class="home-columns">
      <section>
        <div class="section-heading">
          <div>
            <h2>我的课程</h2>
            <p>从教材体系进入章节学习</p>
          </div>
          <el-button text type="primary" @click="router.push('/courses')">查看全部</el-button>
        </div>
        <div v-loading="courseLoading" class="course-list">
          <article v-for="course in courses.slice(0, 3)" :key="course.id" class="course-row" @click="router.push(`/courses/${course.id}`)">
            <img
              v-if="getPrimaryCourseCover(course)"
              :src="getPrimaryCourseCover(course)"
              :alt="`${course.courseName}课程封面`"
              @error="handleImageError"
            />
            <div class="course-fallback"><el-icon><Collection /></el-icon></div>
            <div class="course-row__content">
              <strong>{{ course.courseName }}</strong>
              <p>{{ course.courseDesc || '按章节复习本课程的核心知识。' }}</p>
              <span>学习记录 {{ recordCountForCourse(course.id) }} 条</span>
            </div>
            <el-button type="primary" plain>进入学习</el-button>
          </article>
          <el-empty v-if="!courseLoading && !courses.length" description="暂无已开放课程" />
        </div>
      </section>

      <aside class="recent-panel">
        <div class="section-heading">
          <div>
            <h2>最近学习</h2>
            <p>从上次停下的位置继续</p>
          </div>
        </div>
        <div v-loading="recordsLoading" class="recent-list">
          <button
            v-for="record in records.slice(0, 5)"
            :key="record.recordId"
            type="button"
            @click="router.push(`/questions/${record.questionId}`)"
          >
            <span class="recent-icon"><el-icon><EditPen /></el-icon></span>
            <span>
              <strong>{{ record.title || '未命名题目' }}</strong>
              <small>{{ record.courseName || '课程' }} · {{ record.lastAttemptAt || '最近学习' }}</small>
            </span>
            <el-tag size="small" :type="record.status === 'mastered' ? 'success' : 'info'">
              {{ record.status === 'mastered' ? '已掌握' : '已学习' }}
            </el-tag>
          </button>
          <el-empty v-if="!recordsLoading && !records.length" description="完成一道题后，这里会出现学习记录" />
        </div>
      </aside>
    </div>

    <section class="quick-section">
      <div class="section-heading">
        <div>
          <h2>快捷入口</h2>
          <p>直接进入最常用的复习功能</p>
        </div>
      </div>
      <div class="quick-links">
        <button v-for="item in quickLinks" :key="item.path" type="button" @click="router.push(item.path)">
          <span :class="item.tone"><el-icon><component :is="item.icon" /></el-icon></span>
          <strong>{{ item.label }}</strong>
          <small>{{ item.description }}</small>
          <el-icon class="quick-arrow"><ArrowRight /></el-icon>
        </button>
      </div>
    </section>
  </div>
</template>

<script setup>
import { computed, onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'
import StatCard from '../../components/StatCard.vue'
import { getCourseList, getQuestionRecords, getWrongQuestions } from '../../api/course'
import { getPrimaryCourseCover } from '../../data/courseTextbooks'
import { useUserStore } from '../../stores/user'

const router = useRouter()
const userStore = useUserStore()
const courses = ref([])
const records = ref([])
const wrongQuestions = ref([])
const courseLoading = ref(false)
const recordsLoading = ref(false)

const masteredCount = computed(() => records.value.filter((item) => item.status === 'mastered').length)
const greeting = computed(() => {
  const hour = new Date().getHours()
  if (hour < 11) return '早上好'
  if (hour < 14) return '中午好'
  if (hour < 18) return '下午好'
  return '晚上好'
})

const quickLinks = [
  { label: '章节知识点', description: '复习概念、公式和易错点', path: '/courses', icon: 'Reading', tone: 'blue' },
  { label: '题库练习', description: '按课程、题型和难度筛选', path: '/questions', icon: 'EditPen', tone: 'green' },
  { label: '错题本', description: '集中回看需要加强的题目', path: '/study?tab=wrong', icon: 'Warning', tone: 'red' },
  { label: '往年试题', description: '查看公开试卷和个人试卷库', path: '/papers', icon: 'Document', tone: 'orange' }
]

onMounted(async () => {
  courseLoading.value = true
  recordsLoading.value = true
  const [courseResult, recordResult, wrongResult] = await Promise.allSettled([
    getCourseList({ page: 1, pageSize: 100 }),
    getQuestionRecords({ page: 1, pageSize: 50 }),
    getWrongQuestions({ page: 1, pageSize: 50 })
  ])
  if (courseResult.status === 'fulfilled') courses.value = courseResult.value.data?.items || []
  if (recordResult.status === 'fulfilled') records.value = recordResult.value.data?.items || []
  if (wrongResult.status === 'fulfilled') wrongQuestions.value = wrongResult.value.data?.items || []
  courseLoading.value = false
  recordsLoading.value = false
})

function recordCountForCourse(courseId) {
  return records.value.filter((item) => Number(item.courseId) === Number(courseId)).length
}

function handleImageError(event) {
  event.currentTarget.style.display = 'none'
}
</script>

<style scoped>
.home-page {
  display: grid;
  gap: 24px;
}

.welcome-panel {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 320px;
  align-items: center;
  gap: 32px;
  min-height: 220px;
  padding: 32px 36px;
  border-radius: var(--radius-xl);
  color: #fff;
  background: var(--color-primary);
}

.welcome-copy > span {
  font-size: var(--text-sm);
  opacity: 0.86;
}

.welcome-copy h1 {
  margin: 8px 0 10px;
  font-size: 30px;
}

.welcome-copy p {
  max-width: 620px;
  color: rgba(255, 255, 255, 0.86);
}

.welcome-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 22px;
}

.welcome-actions :deep(.el-button--primary) {
  color: var(--color-primary);
  background: #fff;
  border-color: #fff;
}

.welcome-actions :deep(.el-button:not(.el-button--primary)) {
  color: #fff;
  background: transparent;
  border-color: rgba(255, 255, 255, 0.46);
}

.study-route {
  padding: 20px;
  border-radius: var(--radius-lg);
  color: var(--color-text-primary);
  background: #fff;
}

.study-route > span {
  color: var(--color-text-secondary);
  font-size: var(--text-xs);
}

.study-route ol {
  display: grid;
  gap: 12px;
  margin-top: 14px;
  list-style: none;
}

.study-route li {
  display: flex;
  align-items: center;
  gap: 10px;
  font-weight: 600;
}

.study-route strong {
  display: inline-flex;
  width: 26px;
  height: 26px;
  align-items: center;
  justify-content: center;
  border-radius: 50%;
  color: var(--color-primary);
  background: var(--color-primary-light);
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 14px;
}

.home-columns {
  display: grid;
  grid-template-columns: minmax(0, 1.45fr) minmax(320px, 0.75fr);
  gap: 20px;
}

.home-columns > section,
.recent-panel,
.quick-section {
  padding: 22px;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-xl);
  background: var(--color-surface);
}

.course-list,
.recent-list {
  display: grid;
  gap: 10px;
}

.course-row {
  display: grid;
  grid-template-columns: 76px minmax(0, 1fr) auto;
  align-items: center;
  gap: 14px;
  min-height: 100px;
  padding: 12px;
  border-radius: var(--radius-lg);
  background: var(--color-surface-muted);
  cursor: pointer;
  transition: background var(--motion-fast);
}

.course-row:hover {
  background: var(--color-primary-light);
}

.course-row img,
.course-fallback {
  grid-area: 1 / 1;
  width: 76px;
  height: 76px;
  border-radius: var(--radius-md);
  object-fit: cover;
}

.course-fallback {
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--color-primary);
  background: var(--color-primary-light);
}

.course-row img {
  position: relative;
  z-index: 1;
}

.course-row__content {
  min-width: 0;
}

.course-row__content strong {
  font-size: var(--text-md);
}

.course-row__content p {
  display: -webkit-box;
  overflow: hidden;
  margin: 4px 0;
  color: var(--color-text-secondary);
  font-size: var(--text-sm);
  -webkit-box-orient: vertical;
  -webkit-line-clamp: 1;
}

.course-row__content span {
  color: var(--color-primary);
  font-size: var(--text-xs);
}

.recent-list button {
  display: grid;
  grid-template-columns: 36px minmax(0, 1fr) auto;
  align-items: center;
  gap: 10px;
  width: 100%;
  min-height: 62px;
  padding: 8px;
  border: 0;
  border-radius: var(--radius-md);
  text-align: left;
  background: transparent;
  cursor: pointer;
}

.recent-list button:hover {
  background: var(--color-surface-muted);
}

.recent-icon {
  display: inline-flex;
  width: 36px;
  height: 36px;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-md);
  color: var(--color-primary);
  background: var(--color-primary-light);
}

.recent-list strong,
.recent-list small {
  display: block;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.recent-list strong {
  font-size: var(--text-sm);
}

.recent-list small {
  margin-top: 2px;
  color: var(--color-text-secondary);
  font-size: 11px;
}

.quick-links {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 12px;
}

.quick-links button {
  position: relative;
  display: grid;
  grid-template-columns: 42px minmax(0, 1fr) 20px;
  grid-template-rows: auto auto;
  align-items: center;
  gap: 2px 12px;
  min-height: 84px;
  padding: 14px;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  text-align: left;
  background: var(--color-surface);
  cursor: pointer;
}

.quick-links button:hover {
  border-color: var(--color-primary);
}

.quick-links button > span:first-child {
  display: inline-flex;
  grid-row: 1 / 3;
  width: 42px;
  height: 42px;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-md);
  color: var(--color-primary);
  background: var(--color-primary-light);
}

.quick-links button > span.green {
  color: #15803d;
  background: var(--color-success-light);
}

.quick-links button > span.red {
  color: #dc2626;
  background: var(--color-danger-light);
}

.quick-links button > span.orange {
  color: #b45309;
  background: var(--color-warning-light);
}

.quick-links small {
  color: var(--color-text-secondary);
  font-size: 11px;
}

.quick-arrow {
  grid-area: 1 / 3 / 3 / 4;
  color: var(--color-text-muted);
}

@media (max-width: 1100px) {
  .stats-grid,
  .quick-links {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .home-columns {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 760px) {
  .welcome-panel {
    grid-template-columns: 1fr;
    min-height: 0;
    padding: 24px 20px;
  }

  .study-route {
    display: none;
  }

  .welcome-copy h1 {
    font-size: 24px;
  }

  .stats-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 10px;
  }

  .home-columns > section,
  .recent-panel,
  .quick-section {
    padding: 16px;
  }

  .course-row {
    grid-template-columns: 62px minmax(0, 1fr);
  }

  .course-row img,
  .course-fallback {
    width: 62px;
    height: 70px;
  }

  .course-row .el-button {
    display: none;
  }

  .quick-links {
    grid-template-columns: 1fr;
  }
}
</style>
