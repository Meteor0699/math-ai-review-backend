<template>
  <div class="course-list-page">
    <div class="page-header">
      <h2>
        <el-icon><Collection /></el-icon>
        课程列表
      </h2>
      <p class="page-desc">选择课程，开始系统化复习</p>
      <el-divider />
    </div>

    <div v-loading="loading" class="course-grid">
      <el-card
        v-for="course in courseList"
        :key="course.id"
        class="course-card"
        shadow="hover"
        :body-style="{ padding: '32px 28px' }"
        @click="$router.push(`/courses/${course.id}`)"
      >
        <div class="card-top">
          <div class="course-icon" :style="{ background: courseColors[course.id % 3] }">
            <el-icon :size="28"><Notebook /></el-icon>
          </div>
          <el-tag :type="tagTypes[course.id % 3]" size="small" effect="plain">
            {{ course.code }}
          </el-tag>
        </div>
        <div class="course-name">{{ course.name }}</div>
        <div class="course-desc">{{ course.description }}</div>
        <el-divider style="margin: 16px 0" />
        <div class="card-footer">
          <span class="enter-link">
            进入课程 <el-icon><ArrowRight /></el-icon>
          </span>
        </div>
      </el-card>
    </div>

    <el-empty v-if="!loading && courseList.length === 0" description="暂无课程数据" />
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { getCourseList } from '../../api/course'
import { ElMessage } from 'element-plus'

const courseColors = [
  'linear-gradient(135deg, #667eea 0%, #764ba2 100%)',
  'linear-gradient(135deg, #f093fb 0%, #f5576c 100%)',
  'linear-gradient(135deg, #4facfe 0%, #00f2fe 100%)'
]

const tagTypes = ['', 'danger', 'success']

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
  max-width: 1100px;
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
  grid-template-columns: repeat(3, 1fr);
  gap: 24px;
}

.course-card {
  cursor: pointer;
  border-radius: 10px;
  transition: transform 0.25s, box-shadow 0.25s;
}

.course-card:hover {
  transform: translateY(-6px);
}

.card-top {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: 16px;
}

.course-icon {
  display: inline-flex;
  justify-content: center;
  align-items: center;
  width: 52px;
  height: 52px;
  border-radius: 12px;
  color: #fff;
}

.course-name {
  font-size: 20px;
  font-weight: 700;
  color: #303133;
  margin-bottom: 8px;
}

.course-desc {
  font-size: 13px;
  color: #909399;
  line-height: 1.6;
  min-height: 40px;
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

@media (max-width: 900px) {
  .course-grid {
    grid-template-columns: repeat(2, 1fr);
  }
}

@media (max-width: 600px) {
  .course-grid {
    grid-template-columns: 1fr;
  }
}
</style>