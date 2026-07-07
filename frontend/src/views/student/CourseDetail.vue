<template>
  <div class="course-detail">
    <!-- 面包屑 -->
    <el-breadcrumb separator="/" style="margin-bottom: 16px">
      <el-breadcrumb-item :to="{ path: '/courses' }">课程列表</el-breadcrumb-item>
      <el-breadcrumb-item>{{ course?.courseName || '课程详情' }}</el-breadcrumb-item>
    </el-breadcrumb>

    <!-- 课程信息 -->
    <div v-loading="courseLoading">
      <el-card v-if="course" class="info-card" shadow="never">
        <div class="info-header">
          <div class="info-icon" :style="{ background: courseColors[(course.id % 3)] }">
            <el-icon :size="28"><Notebook /></el-icon>
          </div>
          <div class="info-text">
            <h2>{{ course.courseName }}</h2>
            <el-tag size="small" effect="plain">{{ course.courseCode }}</el-tag>
          </div>
        </div>
        <el-divider style="margin: 16px 0" />
        <p class="info-desc">{{ course.courseDesc }}</p>
      </el-card>
      <el-empty v-if="!courseLoading && !course" description="课程不存在" />
    </div>

    <!-- 章节列表 -->
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
import { ref, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { getCourseDetail, getChapters } from '../../api/course'
import { ElMessage } from 'element-plus'

const route = useRoute()
const courseId = route.params.courseId

const courseColors = [
  'linear-gradient(135deg, #667eea 0%, #764ba2 100%)',
  'linear-gradient(135deg, #f093fb 0%, #f5576c 100%)',
  'linear-gradient(135deg, #4facfe 0%, #00f2fe 100%)'
]

const course = ref(null)
const chapters = ref([])
const courseLoading = ref(false)
const chaptersLoading = ref(false)

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
  max-width: 900px;
  margin: 0 auto;
}

/* 课程信息卡片 */
.info-card {
  border-radius: 10px;
}

.info-header {
  display: flex;
  align-items: center;
  gap: 16px;
}

.info-icon {
  display: inline-flex;
  justify-content: center;
  align-items: center;
  width: 52px;
  height: 52px;
  border-radius: 12px;
  color: #fff;
  flex-shrink: 0;
}

.info-text h2 {
  font-size: 22px;
  font-weight: 700;
  color: #303133;
  margin-bottom: 6px;
}

.info-desc {
  font-size: 14px;
  color: #606266;
  line-height: 1.6;
}

/* 章节列表 */
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
</style>