<template>
  <div class="course-detail">
    <el-breadcrumb separator="/" style="margin-bottom: 16px">
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
          <div v-else class="info-icon" :style="{ background: courseColors[(course.id % 3)] }">
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

    <div class="section-header" style="margin-top: 28px">
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
        :body-style="{ padding: '20px 24px' }"
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

const courseColors = [
  'linear-gradient(135deg, #2f80ed 0%, #56ccf2 100%)',
  'linear-gradient(135deg, #27ae60 0%, #6fcf97 100%)',
  'linear-gradient(135deg, #9b51e0 0%, #bb6bd9 100%)'
]

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
  max-width: 980px;
  margin: 0 auto;
}

.course-hero {
  display: grid;
  grid-template-columns: 180px 1fr;
  gap: 24px;
  align-items: stretch;
  border: 1px solid #ebeef5;
  border-radius: 8px;
  background: #fff;
  overflow: hidden;
}

.hero-cover {
  min-height: 240px;
  background: #f5f7fa;
  display: flex;
  align-items: center;
  justify-content: center;
  border-right: 1px solid #ebeef5;
}

.hero-cover img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.hero-main {
  padding: 28px 28px 24px 0;
}

.hero-title-row {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 16px;
}

.hero-title-row h2 {
  font-size: 24px;
  font-weight: 700;
  color: #303133;
}

.info-icon {
  display: inline-flex;
  justify-content: center;
  align-items: center;
  width: 72px;
  height: 72px;
  border-radius: 8px;
  color: #fff;
  flex-shrink: 0;
}

.info-desc {
  font-size: 14px;
  color: #606266;
  line-height: 1.8;
}

.textbook-section {
  margin-top: 28px;
}

.section-header {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 16px;
}

.section-header h3 {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 18px;
  color: #303133;
}

.section-count {
  font-size: 13px;
  color: #909399;
}

.textbook-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(190px, 1fr));
  gap: 16px;
}

.textbook-card {
  border: 1px solid #ebeef5;
  border-radius: 8px;
  background: #fff;
  overflow: hidden;
}

.textbook-cover {
  display: block;
  width: 100%;
  aspect-ratio: 0.72;
  object-fit: cover;
  background: #f5f7fa;
}

.textbook-info {
  padding: 12px 14px 14px;
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.textbook-info strong {
  font-size: 15px;
  color: #303133;
}

.textbook-info span,
.textbook-info small {
  color: #909399;
  font-size: 12px;
}

.chapter-card {
  margin-bottom: 12px;
  border-radius: 8px;
  transition: transform 0.2s;
}

.chapter-card:hover {
  transform: translateX(4px);
}

.chapter-main {
  display: flex;
  align-items: center;
  gap: 16px;
  cursor: pointer;
}

.chapter-index {
  display: flex;
  justify-content: center;
  align-items: center;
  width: 36px;
  height: 36px;
  border-radius: 50%;
  background: #ecf5ff;
  color: #409EFF;
  font-size: 15px;
  font-weight: 700;
  flex-shrink: 0;
}

.chapter-info {
  flex: 1;
}

.chapter-title {
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.chapter-desc {
  font-size: 13px;
  color: #909399;
  margin-top: 4px;
}

.chapter-arrow {
  color: #c0c4cc;
  font-size: 16px;
}

.chapter-actions {
  margin-top: 12px;
  padding-top: 12px;
  border-top: 1px dashed #e4e7ed;
  display: flex;
  gap: 8px;
}

@media (max-width: 720px) {
  .course-hero {
    grid-template-columns: 1fr;
  }

  .hero-cover {
    height: 260px;
    border-right: 0;
    border-bottom: 1px solid #ebeef5;
  }

  .hero-main {
    padding: 22px;
  }
}
</style>
