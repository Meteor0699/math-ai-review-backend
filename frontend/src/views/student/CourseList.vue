<template>
  <div class="app-page">
    <PageHeader title="课程学习" description="按课程进入章节目录，沿着教材体系完成复习。">
      <template #actions>
        <el-button :icon="EditPen" @click="$router.push('/questions')">进入题库</el-button>
      </template>
    </PageHeader>

    <div v-loading="loading" class="course-grid">
      <article v-for="course in courses" :key="course.id" class="course-card">
        <div class="cover-area">
          <img
            v-if="getPrimaryCourseCover(course)"
            :src="getPrimaryCourseCover(course)"
            :alt="`${course.courseName || course.name}教材封面`"
            @error="hideBrokenImage"
          />
          <span class="cover-fallback"><el-icon><Collection /></el-icon></span>
        </div>
        <div class="course-body">
          <div class="course-heading">
            <div>
              <span>{{ course.courseCode || course.code || '大学数学' }}</span>
              <h2>{{ course.courseName || course.name }}</h2>
            </div>
            <el-tag effect="plain">{{ chapterCounts[course.id] || 0 }} 章</el-tag>
          </div>
          <p>{{ course.courseDesc || course.description || '按章节复习本课程的核心概念、公式和典型题。' }}</p>
          <div class="course-meta">
            <span><el-icon><Reading /></el-icon>{{ getCourseTextbooks(course).length }} 本参考教材</span>
            <span><el-icon><Notebook /></el-icon>{{ recordCount(course.id) }} 条学习记录</span>
          </div>
          <el-button type="primary" @click="$router.push(`/courses/${course.id}`)">进入学习</el-button>
        </div>
      </article>
    </div>

    <el-empty v-if="!loading && !courses.length" description="暂无已开放课程" />
  </div>
</template>

<script setup>
import { onMounted, reactive, ref } from 'vue'
import { EditPen } from '@element-plus/icons-vue'
import PageHeader from '../../components/PageHeader.vue'
import { getChapters, getCourseList, getQuestionRecords } from '../../api/course'
import { getCourseTextbooks, getPrimaryCourseCover } from '../../data/courseTextbooks'

const courses = ref([])
const records = ref([])
const chapterCounts = reactive({})
const loading = ref(false)

onMounted(async () => {
  loading.value = true
  try {
    const [courseResponse, recordResponse] = await Promise.all([
      getCourseList({ page: 1, pageSize: 100 }),
      getQuestionRecords({ page: 1, pageSize: 100 }).catch(() => ({ data: { items: [] } }))
    ])
    courses.value = courseResponse.data?.items || []
    records.value = recordResponse.data?.items || []
    await Promise.all(courses.value.map(async (course) => {
      const response = await getChapters(course.id, { page: 1, pageSize: 100 }).catch(() => null)
      chapterCounts[course.id] = response?.data?.items?.length || 0
    }))
  } finally {
    loading.value = false
  }
})

function recordCount(courseId) {
  return records.value.filter((item) => Number(item.courseId) === Number(courseId)).length
}

function hideBrokenImage(event) {
  event.currentTarget.style.display = 'none'
}
</script>

<style scoped>
.course-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(310px, 1fr));
  gap: 18px;
}

.course-card {
  display: grid;
  grid-template-columns: 116px minmax(0, 1fr);
  min-height: 224px;
  overflow: hidden;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-xl);
  background: var(--color-surface);
  transition: transform var(--motion-base) var(--ease-out), border-color var(--motion-fast);
}

.course-card:hover {
  transform: translateY(-3px);
  border-color: var(--color-primary);
}

.cover-area {
  position: relative;
  min-height: 224px;
  background: var(--color-primary-light);
}

.cover-area img,
.cover-fallback {
  position: absolute;
  inset: 0;
  width: 100%;
  height: 100%;
}

.cover-area img {
  z-index: 1;
  object-fit: cover;
}

.cover-fallback {
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--color-primary);
  font-size: 32px;
}

.course-body {
  display: flex;
  min-width: 0;
  flex-direction: column;
  align-items: flex-start;
  padding: 20px;
}

.course-heading {
  display: flex;
  width: 100%;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
}

.course-heading span {
  color: var(--color-primary);
  font-size: var(--text-xs);
}

.course-heading h2 {
  margin-top: 3px;
  font-size: var(--text-lg);
}

.course-body > p {
  display: -webkit-box;
  overflow: hidden;
  margin: 12px 0;
  color: var(--color-text-secondary);
  font-size: var(--text-sm);
  -webkit-box-orient: vertical;
  -webkit-line-clamp: 3;
}

.course-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
  margin: auto 0 14px;
  color: var(--color-text-secondary);
  font-size: var(--text-xs);
}

.course-meta span {
  display: inline-flex;
  align-items: center;
  gap: 4px;
}

@media (max-width: 520px) {
  .course-grid {
    grid-template-columns: 1fr;
  }

  .course-card {
    grid-template-columns: 90px minmax(0, 1fr);
  }

  .cover-area {
    min-height: 230px;
  }
}
</style>
