<template>
  <div class="home-page">
    <section class="hero-panel">
      <div>
        <span class="eyebrow">大学数学复习空间</span>
        <h1>今天先把一个知识点吃透</h1>
        <p>按课程进入章节，先看重点知识，再用题库和 AI 讲解补齐理解。</p>
      </div>
      <div class="today-card">
        <span>建议顺序</span>
        <strong>知识点 → 例题 → 追问</strong>
        <small>适合考前 20-40 分钟一轮复盘</small>
      </div>
    </section>

    <section class="section">
      <div class="section-heading">
        <div>
          <h2>课程选择</h2>
          <p>选择你正在复习的课程</p>
        </div>
        <el-button text type="primary" @click="router.push('/courses')">
          全部课程 <el-icon><ArrowRight /></el-icon>
        </el-button>
      </div>

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
              <span>进入学习</span>
              <el-icon><ArrowRight /></el-icon>
            </div>
          </div>
        </el-card>
      </div>
    </section>

    <section class="section">
      <div class="section-heading">
        <div>
          <h2>快捷功能</h2>
          <p>复习时最常用的入口</p>
        </div>
      </div>
      <div class="feature-grid">
        <el-card
          v-for="item in featureList"
          :key="item.path"
          class="feature-card"
          shadow="hover"
          :body-style="{ padding: '22px' }"
          @click="router.push(item.path)"
        >
          <div class="feature-icon" :class="item.tone">
            <el-icon :size="22"><component :is="item.icon" /></el-icon>
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
    desc: '先抓公式、定理和易错点',
    path: '/courses',
    icon: 'Reading',
    tone: 'blue'
  },
  {
    name: '题库练习',
    desc: '按章节筛题，边做边查解析',
    path: '/questions',
    icon: 'Edit',
    tone: 'green'
  },
  {
    name: '往年试题',
    desc: '查看课程历年试卷资料',
    path: '/papers',
    icon: 'Document',
    tone: 'orange'
  },
  {
    name: 'AI 讲解',
    desc: '题目看不懂时继续追问',
    path: '/questions',
    icon: 'MagicStick',
    tone: 'purple'
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

.hero-panel {
  display: grid;
  grid-template-columns: 1fr 280px;
  gap: 24px;
  align-items: center;
  border: 1px solid #dbe6f8;
  border-radius: 8px;
  background: #fff;
  padding: 30px;
  margin-bottom: 34px;
}

.eyebrow {
  display: inline-block;
  color: #2563eb;
  background: #eaf2ff;
  border-radius: 999px;
  padding: 5px 10px;
  font-size: 12px;
  margin-bottom: 14px;
}

.hero-panel h1 {
  font-size: 30px;
  color: #1f2937;
  margin-bottom: 10px;
}

.hero-panel p {
  color: #5b6472;
  line-height: 1.7;
}

.today-card {
  border: 1px solid #e5e7eb;
  border-radius: 8px;
  padding: 20px;
  background: #f9fbff;
}

.today-card span,
.today-card small {
  display: block;
  color: #6b7280;
  font-size: 13px;
}

.today-card strong {
  display: block;
  color: #1f2937;
  font-size: 18px;
  margin: 10px 0;
}

.section {
  margin-bottom: 34px;
}

.section-heading {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  margin-bottom: 18px;
}

.section-heading h2 {
  font-size: 20px;
  color: #1f2937;
}

.section-heading p {
  color: #909399;
  font-size: 14px;
  margin-top: 4px;
}

.course-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 20px;
}

.course-card,
.feature-card {
  cursor: pointer;
  overflow: hidden;
  transition: transform 0.22s, box-shadow 0.22s, border-color 0.22s;
}

.course-card:hover,
.feature-card:hover {
  transform: translateY(-4px);
  border-color: #cfe0ff;
}

.course-cover {
  display: block;
  width: 100%;
  height: 188px;
  object-fit: cover;
  background: #f5f7fa;
  border-bottom: 1px solid #ebeef5;
}

.course-content {
  padding: 18px 20px 20px;
}

.course-name {
  font-size: 18px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 4px;
}

.course-code {
  font-size: 13px;
  color: #2563eb;
  margin-bottom: 10px;
}

.course-desc {
  font-size: 13px;
  color: #606266;
  line-height: 1.55;
  margin-bottom: 16px;
  min-height: 58px;
}

.course-action {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 13px;
  color: #2563eb;
  font-weight: 600;
}

.feature-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 18px;
}

.feature-icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 44px;
  height: 44px;
  border-radius: 8px;
  margin-bottom: 14px;
}

.feature-icon.blue {
  color: #2563eb;
  background: #eaf2ff;
}

.feature-icon.green {
  color: #16a34a;
  background: #eaf8ef;
}

.feature-icon.orange {
  color: #c97706;
  background: #fff5dd;
}

.feature-icon.purple {
  color: #7c3aed;
  background: #f0eaff;
}

.feature-name {
  font-size: 16px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 6px;
}

.feature-desc {
  font-size: 13px;
  color: #6b7280;
  line-height: 1.5;
}

@media (max-width: 920px) {
  .hero-panel {
    grid-template-columns: 1fr;
  }

  .course-grid,
  .feature-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 640px) {
  .hero-panel {
    padding: 22px;
  }

  .hero-panel h1 {
    font-size: 24px;
  }

  .course-grid,
  .feature-grid {
    grid-template-columns: 1fr;
  }
}
</style>
