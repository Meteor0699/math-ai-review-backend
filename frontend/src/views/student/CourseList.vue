<template>
  <div class="course-list-page">
    <div class="page-header">
      <h2>
        <el-icon><Collection /></el-icon>
        课程列表
      </h2>
      <p class="page-desc">选择课程，按教材体系开始复习</p>
      <el-divider />
    </div>

    <div v-loading="loading" class="course-grid">
      <el-card
        v-for="course in courseList"
        :key="course.id"
        class="course-card"
        shadow="hover"
        :body-style="{ padding: 0 }"
        @click="$router.push(`/courses/${course.id}`)"
      >
        <div class="cover-wrap">
          <img
            v-if="getPrimaryCourseCover(course)"
            class="course-cover"
            :src="getPrimaryCourseCover(course)"
            :alt="`${course.courseName || course.name} 课程封面`"
          />
          <div v-else class="course-icon" :style="{ background: courseColors[course.id % 3] }">
            <el-icon :size="28"><Notebook /></el-icon>
          </div>
        </div>
        <div class="card-body">
          <div class="card-top">
            <el-tag :type="tagTypes[course.id % 3]" size="small" effect="plain">
              {{ course.courseCode || course.code }}
            </el-tag>
            <span class="book-count">{{ getCourseTextbooks(course).length }} 本参考教材</span>
          </div>
          <div class="course-name">{{ course.courseName || course.name }}</div>
          <div class="course-desc">{{ course.courseDesc || course.description }}</div>
          <el-divider style="margin: 16px 0" />
          <div class="card-footer">
            <span class="enter-link">
              进入课程 <el-icon><ArrowRight /></el-icon>
            </span>
          </div>
        </div>
      </el-card>
    </div>

    <el-empty v-if="!loading && courseList.length === 0" description="暂无课程数据" />
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { getCourseList } from '../../api/course'
import { getCourseTextbooks, getPrimaryCourseCover } from '../../data/courseTextbooks'
import { ElMessage } from 'element-plus'

const courseColors = [
  'linear-gradient(135deg, #2f80ed 0%, #56ccf2 100%)',
  'linear-gradient(135deg, #27ae60 0%, #6fcf97 100%)',
  'linear-gradient(135deg, #9b51e0 0%, #bb6bd9 100%)'
]

const tagTypes = ['', 'success', 'warning']

const courseList = ref([])
const loading = ref(false)

onMounted(async () => {
  loading.value = true
  try {
    const res = await getCourseList({ page: 1, pageSize: 100 })
    if (res.code === 200) {
      courseList.value = res.data?.items || []
    }
  } catch {
    ElMessage.error('加载课程列表失败')
  } finally {
    loading.value = false
  }
})
</script>

<style scoped>
.course-list-page {
  max-width: 1120px;
  margin: 0 auto;
}

.page-header h2 {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 22px;
  color: #303133;
}

.page-desc {
  font-size: 14px;
  color: #909399;
  margin-top: 6px;
}

.course-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 24px;
}

.course-card {
  cursor: pointer;
  border-radius: 8px;
  overflow: hidden;
  transition: transform 0.25s, box-shadow 0.25s;
}

.course-card:hover {
  transform: translateY(-5px);
}

.cover-wrap {
  height: 210px;
  background: #f5f7fa;
  display: flex;
  align-items: center;
  justify-content: center;
  border-bottom: 1px solid #ebeef5;
}

.course-cover {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.course-icon {
  display: inline-flex;
  justify-content: center;
  align-items: center;
  width: 72px;
  height: 72px;
  border-radius: 8px;
  color: #fff;
}

.card-body {
  padding: 20px 22px 22px;
}

.card-top {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 12px;
  margin-bottom: 12px;
}

.book-count {
  color: #909399;
  font-size: 12px;
  white-space: nowrap;
}

.course-name {
  font-size: 20px;
  font-weight: 700;
  color: #303133;
  margin-bottom: 8px;
}

.course-desc {
  font-size: 13px;
  color: #606266;
  line-height: 1.6;
  min-height: 64px;
}

.card-footer {
  text-align: right;
}

.enter-link {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  font-size: 13px;
  color: #409EFF;
  font-weight: 500;
}

@media (max-width: 960px) {
  .course-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 640px) {
  .course-grid {
    grid-template-columns: 1fr;
  }
}
</style>
