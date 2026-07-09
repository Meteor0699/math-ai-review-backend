<template>
  <div class="home-page">
    <section class="hero-banner">
      <div class="hero-content">
        <h1 class="hero-title">
          <el-icon :size="34"><School /></el-icon>
          数学 AI 复习助手
        </h1>
        <p class="hero-desc">系统化复习 · 重点知识梳理 · AI 智能讲解 · 往年试题精练</p>
      </div>
    </section>

    <section class="section">
      <h2 class="section-title">
        <el-icon><Collection /></el-icon>
        课程选择
      </h2>
      <p class="section-desc">选择你需要复习的课程，按教材与章节进入学习</p>
      <div class="course-grid" v-loading="courseLoading">
        <el-card
          v-for="course in courseList"
          :key="course.id"
          class="course-card"
          shadow="hover"
          :body-style="{ padding: 0 }"
          @click="router.push(`/courses/${course.id}`)"
        >
          <img
            v-if="getPrimaryCourseCover(course)"
            class="course-cover"
            :src="getPrimaryCourseCover(course)"
            :alt="course.courseName"
          />
          <div class="course-content">
            <div class="course-name">{{ course.courseName }}</div>
            <div class="course-code">{{ course.courseCode }}</div>
            <div class="course-desc">{{ course.courseDesc }}</div>
            <div class="course-action">
              <span>进入课程</span>
              <el-icon><ArrowRight /></el-icon>
            </div>
          </div>
        </el-card>
      </div>
    </section>

    <section class="section">
      <h2 class="section-title">
        <el-icon><Menu /></el-icon>
        快捷功能
      </h2>
      <p class="section-desc">快速进入常用复习模块</p>
      <div class="feature-grid">
        <el-card
          v-for="item in featureList"
          :key="item.path"
          class="feature-card"
          shadow="hover"
          :body-style="{ padding: '24px' }"
          @click="router.push(item.path)"
        >
          <div class="feature-icon" :style="{ background: item.color }">
            <el-icon :size="24"><component :is="item.icon" /></el-icon>
          </div>
          <div class="feature-name">{{ item.name }}</div>
          <div class="feature-desc">{{ item.desc }}</div>
        </el-card>
      </div>
    </section>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { getCourseList } from '../../api/course'
import { getPrimaryCourseCover } from '../../data/courseTextbooks'

const router = useRouter()

const courseList = ref([])
const courseLoading = ref(false)

const featureList = [
  {
    name: '重点知识',
    desc: '查看课程核心知识点与公式',
    path: '/courses',
    icon: 'Reading',
    color: 'linear-gradient(135deg, #2f80ed 0%, #56ccf2 100%)'
  },
  {
    name: '题库',
    desc: '按章节、难度筛选练习题目',
    path: '/questions',
    icon: 'Edit',
    color: 'linear-gradient(135deg, #27ae60 0%, #6fcf97 100%)'
  },
  {
    name: '往年试题',
    desc: '浏览历年考试真题与文件',
    path: '/papers',
    icon: 'Document',
    color: 'linear-gradient(135deg, #f2994a 0%, #f2c94c 100%)'
  },
  {
    name: 'AI 讲解',
    desc: '查看题目解析并继续追问',
    path: '/questions',
    icon: 'MagicStick',
    color: 'linear-gradient(135deg, #9b51e0 0%, #bb6bd9 100%)'
  }
]

onMounted(async () => {
  courseLoading.value = true
  try {
    const res = await getCourseList({ page: 1, pageSize: 100 })
    courseList.value = res.data?.items || []
  } catch {
    courseList.value = []
  } finally {
    courseLoading.value = false
  }
})
</script>

<style scoped>
.home-page {
  max-width: 1120px;
  margin: 0 auto;
}

.hero-banner {
  background: linear-gradient(135deg, #2f80ed 0%, #27ae60 100%);
  border-radius: 8px;
  padding: 44px 40px;
  margin-bottom: 40px;
  color: #fff;
}

.hero-content {
  text-align: center;
}

.hero-title {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12px;
  font-size: 30px;
  font-weight: 700;
  margin-bottom: 12px;
}

.hero-desc {
  font-size: 15px;
  opacity: 0.9;
}

.section {
  margin-bottom: 36px;
}

.section-title {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 20px;
  font-weight: 600;
  color: #303133;
  margin-bottom: 6px;
}

.section-desc {
  font-size: 14px;
  color: #909399;
  margin-bottom: 20px;
}

.course-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 20px;
}

.course-card {
  cursor: pointer;
  overflow: hidden;
  border-radius: 8px;
  transition: transform 0.25s, box-shadow 0.25s;
}

.course-card:hover {
  transform: translateY(-5px);
}

.course-cover {
  display: block;
  width: 100%;
  height: 190px;
  object-fit: cover;
  background: #f5f7fa;
  border-bottom: 1px solid #ebeef5;
}

.course-content {
  padding: 18px 20px 20px;
}

.course-name {
  font-size: 18px;
  font-weight: 600;
  color: #303133;
  margin-bottom: 4px;
}

.course-code {
  font-size: 13px;
  color: #409EFF;
  margin-bottom: 10px;
}

.course-desc {
  font-size: 13px;
  color: #606266;
  line-height: 1.5;
  margin-bottom: 16px;
  min-height: 58px;
}

.course-action {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 13px;
  color: #409EFF;
  font-weight: 500;
}

.feature-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 20px;
}

.feature-card {
  cursor: pointer;
  text-align: center;
  transition: transform 0.25s, box-shadow 0.25s;
  border-radius: 8px;
}

.feature-card:hover {
  transform: translateY(-4px);
}

.feature-icon {
  display: inline-flex;
  justify-content: center;
  align-items: center;
  width: 48px;
  height: 48px;
  border-radius: 8px;
  color: #fff;
  margin-bottom: 14px;
}

.feature-name {
  font-size: 16px;
  font-weight: 600;
  color: #303133;
  margin-bottom: 6px;
}

.feature-desc {
  font-size: 13px;
  color: #909399;
  line-height: 1.4;
}

@media (max-width: 900px) {
  .course-grid,
  .feature-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 640px) {
  .course-grid,
  .feature-grid {
    grid-template-columns: 1fr;
  }

  .hero-banner {
    padding: 32px 20px;
  }

  .hero-title {
    font-size: 22px;
  }
}
</style>
