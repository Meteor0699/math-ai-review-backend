<template>
  <div class="chapter-detail">
    <!-- 面包屑 -->
    <el-breadcrumb separator="/" style="margin-bottom: 16px">
      <el-breadcrumb-item :to="{ path: '/courses' }">课程列表</el-breadcrumb-item>
      <el-breadcrumb-item :to="`/courses/${chapter.courseId}`" v-if="chapter.courseId">
        {{ chapter.courseName || '课程' }}
      </el-breadcrumb-item>
      <el-breadcrumb-item>{{ chapter.title || '章节详情' }}</el-breadcrumb-item>
    </el-breadcrumb>

    <!-- 章节信息 -->
    <div v-loading="detailLoading">
      <el-card v-if="chapter.title" class="info-card" shadow="never">
        <div class="info-header">
          <h2>{{ chapter.title }}</h2>
          <el-tag type="primary" effect="plain" size="small">{{ chapter.courseName }}</el-tag>
        </div>
        <el-divider style="margin: 12px 0" />
        <p class="info-desc" v-if="chapter.description">{{ chapter.description }}</p>
        <div class="info-actions">
          <el-button type="primary" @click="$router.push(`/knowledge/${chapterId}`)">
            <el-icon><Reading /></el-icon>
            查看全部重点知识
          </el-button>
          <el-button @click="$router.push(`/questions?chapterId=${chapterId}`)">
            <el-icon><Edit /></el-icon>
            进入题库练习
          </el-button>
        </div>
      </el-card>
      <el-empty v-if="!detailLoading && !chapter.title" description="章节不存在" />
    </div>

    <!-- 重点知识预览 -->
    <div class="section-header" style="margin-top: 28px" v-if="knowledgePoints.length">
      <h3>
        <el-icon><Reading /></el-icon>
        重点知识
      </h3>
      <el-button text type="primary" @click="$router.push(`/knowledge/${chapterId}`)">
        查看全部 <el-icon><ArrowRight /></el-icon>
      </el-button>
    </div>

    <div v-loading="kpLoading">
      <el-card
        v-for="kp in knowledgePoints"
        :key="kp.id"
        class="kp-preview-card"
        shadow="hover"
        :body-style="{ padding: '20px 24px' }"
      >
        <div class="kp-header">
          <div class="kp-title">
            <el-icon><StarFilled /></el-icon>
            {{ kp.title }}
          </div>
          <el-tag
            :type="examFreqType(kp.examFrequency)"
            size="small"
            effect="plain"
          >
            {{ kp.examFrequency || '一般' }}
          </el-tag>
        </div>
        <div class="kp-content" v-if="kp.coreContent">
          {{ truncateText(kp.coreContent, 200) }}
        </div>
        <el-button text type="primary" size="small" style="margin-top: 8px"
          @click="$router.push(`/knowledge/${chapterId}`)">
          查看详情
        </el-button>
      </el-card>
      <el-empty v-if="!kpLoading && knowledgePoints.length === 0" description="暂无知识点" />
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, reactive } from 'vue'
import { useRoute } from 'vue-router'
import { getKnowledgePoints } from '../../api/course'
import { ElMessage } from 'element-plus'

const route = useRoute()
const chapterId = route.params.chapterId

const chapter = reactive({
  title: '',
  description: '',
  courseId: null,
  courseName: ''
})

const knowledgePoints = ref([])
const detailLoading = ref(false)
const kpLoading = ref(false)

onMounted(async () => {
  detailLoading.value = true
  kpLoading.value = true
  try {
    const res = await getKnowledgePoints(chapterId, { page: 1, pageSize: 5 })
    if (res.code === 200) {
      const items = res.data?.items || []
      knowledgePoints.value = items
      if (items.length > 0) {
        chapter.title = items[0].title || ''
        // 章节信息从第一个知识点推测
        if (items[0].chapterId) {
          chapter.courseId = items[0].courseId
        }
      }
    }
  } catch {
    ElMessage.error('加载章节详情失败')
  } finally {
    detailLoading.value = false
    kpLoading.value = false
  }
})

function examFreqType(freq) {
  const map = { high: 'danger', '高频': 'danger', medium: 'warning', '中频': 'warning', low: 'info', '低频': 'info' }
  return map[freq] || 'info'
}

function truncateText(text, maxLen) {
  if (!text) return ''
  return text.length > maxLen ? text.slice(0, maxLen) + '...' : text
}
</script>

<style scoped>
.chapter-detail {
  max-width: 900px;
  margin: 0 auto;
}

/* 章节信息 */
.info-card {
  border-radius: 10px;
}

.info-header {
  display: flex;
  align-items: center;
  gap: 12px;
}

.info-header h2 {
  font-size: 22px;
  font-weight: 700;
  color: #303133;
}

.info-desc {
  font-size: 14px;
  color: #606266;
  line-height: 1.6;
  margin-bottom: 16px;
}

.info-actions {
  display: flex;
  gap: 12px;
}

/* 区块标题 */
.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 16px;
}

.section-header h3 {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 18px;
  color: #303133;
}

/* 知识点预览卡片 */
.kp-preview-card {
  margin-bottom: 12px;
  border-radius: 8px;
  border-left: 4px solid #409EFF;
}

.kp-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 10px;
}

.kp-title {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.kp-title .el-icon {
  color: #f59e0b;
}

.kp-content {
  font-size: 13px;
  color: #606266;
  line-height: 1.6;
}
</style>