<template>
  <div class="app-page course-detail">
    <el-breadcrumb separator="/" class="app-breadcrumb">
      <el-breadcrumb-item :to="{ path: '/courses' }">课程列表</el-breadcrumb-item>
      <el-breadcrumb-item>{{ course?.courseName || '课程详情' }}</el-breadcrumb-item>
    </el-breadcrumb>

    <div v-loading="courseLoading">
      <section v-if="course" class="course-hero">
        <div class="hero-cover">
          <img
            v-if="getPrimaryCourseCover(course)"
            :src="getPrimaryCourseCover(course)"
            :alt="`${course.courseName} 课程封面`"
          />
          <div v-else class="info-icon">
            <el-icon :size="28"><Notebook /></el-icon>
          </div>
        </div>
        <div class="hero-main">
          <div class="hero-title-row">
            <h2>{{ course.courseName }}</h2>
            <el-tag size="small" effect="plain">{{ course.courseCode }}</el-tag>
          </div>
          <p class="info-desc">{{ course.courseDesc }}</p>
        </div>
      </section>
      <el-empty v-if="!courseLoading && !course" description="课程不存在" />
    </div>

    <section v-if="course && textbooks.length" class="textbook-section">
      <div class="section-header">
        <h3>
          <el-icon><Reading /></el-icon>
          参考教材封面
        </h3>
        <span class="section-count">共 {{ textbooks.length }} 本</span>
      </div>
      <div class="textbook-grid">
        <div v-for="book in textbooks" :key="book.coverUrl" class="textbook-card">
          <img class="textbook-cover" :src="book.coverUrl" :alt="book.title" />
          <div class="textbook-info">
            <strong>{{ book.title }}</strong>
            <span>{{ book.subtitle }}</span>
            <small>{{ book.publisher }}</small>
          </div>
        </div>
      </div>
    </section>

    <div class="section-header chapter-section-header">
      <h3>
        <el-icon><Folder /></el-icon>
        章节列表
      </h3>
      <span class="section-count" v-if="chapters.length">共 {{ chapters.length }} 章</span>
    </div>

    <div v-loading="chaptersLoading">
      <el-card
        v-for="(chapter, index) in chapters"
        :key="chapter.id"
        class="chapter-card"
        shadow="hover"
      >
        <div class="chapter-main" @click="$router.push(`/chapters/${chapter.id}`)">
          <div class="chapter-index">{{ index + 1 }}</div>
          <div class="chapter-info">
            <div class="chapter-title">{{ chapter.title }}</div>
            <div class="chapter-desc" v-if="chapter.description">{{ chapter.description }}</div>
          </div>
          <el-icon class="chapter-arrow"><ArrowRight /></el-icon>
        </div>
        <div class="chapter-actions">
          <el-button size="small" text type="primary" @click.stop="$router.push(`/knowledge/${chapter.id}`)">
            <el-icon><Reading /></el-icon>
            重点知识
          </el-button>
          <el-button size="small" text type="primary" @click.stop="$router.push(`/questions?chapterId=${chapter.id}`)">
            <el-icon><Edit /></el-icon>
            题库练习
          </el-button>
        </div>
      </el-card>
      <el-empty v-if="!chaptersLoading && chapters.length === 0" description="暂无章节" />
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { getCourseDetail, getChapters } from '../../api/course'
import { getCourseTextbooks, getPrimaryCourseCover } from '../../data/courseTextbooks'
import { ElMessage } from 'element-plus'

const route = useRoute()
const courseId = route.params.courseId

const course = ref(null)
const chapters = ref([])
const courseLoading = ref(false)
const chaptersLoading = ref(false)
const textbooks = computed(() => getCourseTextbooks(course.value))

onMounted(async () => {
  courseLoading.value = true
  chaptersLoading.value = true
  try {
    const [courseRes, chaptersRes] = await Promise.allSettled([
      getCourseDetail(courseId),
      getChapters(courseId, { page: 1, pageSize: 100 })
    ])
    if (courseRes.status === 'fulfilled' && courseRes.value.code === 200) {
      course.value = courseRes.value.data
    }
    if (chaptersRes.status === 'fulfilled' && chaptersRes.value.code === 200) {
      chapters.value = chaptersRes.value.data?.items || []
    }
  } catch {
    ElMessage.error('加载课程详情失败')
  } finally {
    courseLoading.value = false
    chaptersLoading.value = false
  }
})
</script>

<style scoped>
.course-detail {
  width: min(100%, 1080px);
}

.app-breadcrumb {
  margin-bottom: var(--space-4);
}

.course-hero {
  display: grid;
  grid-template-columns: 152px 1fr;
  gap: var(--space-6);
  align-items: stretch;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-xl);
  background: var(--color-surface);
  overflow: hidden;
}

.hero-cover {
  min-height: 202px;
  background: var(--color-primary-light);
  display: flex;
  align-items: center;
  justify-content: center;
  border-right: 1px solid var(--color-border);
}

.hero-cover img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.hero-main {
  align-self: center;
  padding: var(--space-6) var(--space-6) var(--space-6) 0;
}

.hero-title-row {
  display: flex;
  align-items: center;
  gap: var(--space-3);
  margin-bottom: var(--space-3);
}

.hero-title-row h2 {
  font-size: var(--text-2xl);
  font-weight: 700;
  color: var(--color-text-primary);
}

.info-icon {
  display: inline-flex;
  justify-content: center;
  align-items: center;
  width: 64px;
  height: 64px;
  border-radius: var(--radius-lg);
  color: var(--color-primary);
  background: var(--color-primary-light);
  flex-shrink: 0;
}

.info-desc {
  color: var(--color-text-secondary);
  line-height: 1.8;
}

.textbook-section {
  margin-top: var(--space-6);
}

.section-header {
  display: flex;
  align-items: center;
  gap: var(--space-3);
  margin-bottom: var(--space-4);
}

.section-header h3 {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  font-size: var(--text-lg);
  color: var(--color-text-primary);
}

.section-count {
  font-size: var(--text-sm);
  color: var(--color-text-tertiary);
}

.textbook-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: var(--space-4);
}

.textbook-card {
  display: grid;
  grid-template-columns: 86px 1fr;
  min-height: 122px;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  background: var(--color-surface);
  overflow: hidden;
}

.textbook-cover {
  display: block;
  width: 100%;
  height: 100%;
  object-fit: cover;
  background: var(--color-surface-muted);
}

.textbook-info {
  padding: var(--space-4);
  display: flex;
  flex-direction: column;
  gap: var(--space-1);
}

.textbook-info strong {
  color: var(--color-text-primary);
}

.textbook-info span,
.textbook-info small {
  color: var(--color-text-tertiary);
  font-size: var(--text-xs);
}

.chapter-section-header {
  margin-top: var(--space-7);
}

.chapter-card {
  margin-bottom: var(--space-3);
  border-radius: var(--radius-lg);
  transition: border-color var(--motion-fast) var(--ease-out), box-shadow var(--motion-fast) var(--ease-out);
}

.chapter-card:hover {
  border-color: var(--color-primary-border);
  box-shadow: var(--shadow-sm);
}

.chapter-main {
  display: flex;
  align-items: center;
  gap: var(--space-4);
  padding: var(--space-1) 0;
  cursor: pointer;
}

.chapter-index {
  display: flex;
  justify-content: center;
  align-items: center;
  width: 36px;
  height: 36px;
  border-radius: var(--radius-md);
  background: var(--color-primary-light);
  color: var(--color-primary);
  font-weight: 700;
  flex-shrink: 0;
}

.chapter-info {
  flex: 1;
}

.chapter-title {
  font-size: var(--text-md);
  font-weight: 600;
  color: var(--color-text-primary);
}

.chapter-desc {
  margin-top: var(--space-1);
  color: var(--color-text-tertiary);
  font-size: var(--text-sm);
}

.chapter-arrow {
  color: var(--color-text-tertiary);
  font-size: 16px;
}

.chapter-actions {
  margin-top: var(--space-3);
  padding-top: var(--space-3);
  border-top: 1px solid var(--color-border-light);
  display: flex;
  gap: var(--space-2);
}

@media (max-width: 720px) {
  .course-hero {
    grid-template-columns: 1fr;
  }

  .hero-cover {
    height: 220px;
    border-right: 0;
    border-bottom: 1px solid var(--color-border);
  }

  .hero-main {
    padding: var(--space-5);
  }

  .textbook-grid {
    grid-template-columns: 1fr;
  }

  .chapter-actions {
    flex-wrap: wrap;
  }

  .chapter-actions .el-button {
    min-height: 44px;
  }
}
</style>
