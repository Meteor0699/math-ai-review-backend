<template>
  <div class="app-page chapter-detail">
    <PageHeader
      :eyebrow="chapter.courseName || '课程章节'"
      :title="chapter.title || '章节详情'"
      :description="chapter.description || '集中复习本章重点知识，并通过题库巩固理解。'"
    >
      <template #actions>
        <el-button @click="$router.push(`/questions?chapterId=${chapterId}`)">
          <el-icon><Edit /></el-icon>
          章节题目
        </el-button>
        <el-button type="primary" @click="$router.push(`/knowledge/${chapterId}`)">
          <el-icon><Reading /></el-icon>
          系统复习
        </el-button>
      </template>
    </PageHeader>

    <el-skeleton v-if="detailLoading" :rows="3" animated />

    <template v-else-if="chapter.id">
      <section class="chapter-overview">
        <div class="chapter-number">{{ chapter.sortOrder || '章' }}</div>
        <div>
          <span class="overview-label">本章学习路径</span>
          <p>先梳理核心概念与公式，再结合典型题型和易错点完成针对性练习。</p>
        </div>
      </section>

      <section class="app-section">
        <div class="section-heading">
          <div>
            <h2>重点知识</h2>
            <p>按考试频率梳理本章需要优先掌握的内容</p>
          </div>
          <el-button text type="primary" @click="$router.push(`/knowledge/${chapterId}`)">
            查看完整目录
            <el-icon><ArrowRight /></el-icon>
          </el-button>
        </div>

        <div v-loading="knowledgeLoading" class="knowledge-preview">
          <article v-for="point in knowledgePoints" :key="point.id" class="knowledge-item">
            <div class="knowledge-heading">
              <div class="knowledge-title">
                <el-icon><StarFilled /></el-icon>
                <strong>{{ point.title }}</strong>
              </div>
              <el-tag :type="examFrequencyType(point.examFrequency)" effect="light">
                {{ examFrequencyLabel(point.examFrequency) }}
              </el-tag>
            </div>
            <p>{{ truncateText(point.coreContent, 180) || '该知识点暂未补充概述。' }}</p>
            <el-button text type="primary" @click="$router.push(`/knowledge/${chapterId}?point=${point.id}`)">
              开始复习
              <el-icon><ArrowRight /></el-icon>
            </el-button>
          </article>
          <el-empty
            v-if="!knowledgeLoading && knowledgePoints.length === 0"
            description="本章暂未录入重点知识"
          />
        </div>
      </section>
    </template>

    <el-empty v-else description="未找到该章节" />
  </div>
</template>

<script setup>
import { onMounted, reactive, ref } from 'vue'
import { useRoute } from 'vue-router'
import { ElMessage } from 'element-plus'
import PageHeader from '../../components/PageHeader.vue'
import { getChapterDetail, getKnowledgePoints } from '../../api/course'

const route = useRoute()
const chapterId = route.params.chapterId
const detailLoading = ref(false)
const knowledgeLoading = ref(false)
const knowledgePoints = ref([])
const chapter = reactive({
  id: null,
  title: '',
  description: '',
  courseId: null,
  courseName: '',
  sortOrder: null
})

onMounted(async () => {
  detailLoading.value = true
  knowledgeLoading.value = true

  const [chapterResponse, knowledgeResponse] = await Promise.allSettled([
    getChapterDetail(chapterId),
    getKnowledgePoints(chapterId, { page: 1, pageSize: 6 })
  ])

  if (chapterResponse.status === 'fulfilled' && chapterResponse.value.code === 200) {
    Object.assign(chapter, chapterResponse.value.data)
  } else {
    ElMessage.error('章节信息加载失败')
  }

  if (knowledgeResponse.status === 'fulfilled' && knowledgeResponse.value.code === 200) {
    knowledgePoints.value = knowledgeResponse.value.data?.items || []
  }

  detailLoading.value = false
  knowledgeLoading.value = false
})

function examFrequencyLabel(frequency) {
  return {
    high: '高频考点',
    高频: '高频考点',
    medium: '中频考点',
    中频: '中频考点',
    low: '基础了解',
    低频: '基础了解'
  }[frequency] || frequency || '待标注'
}

function examFrequencyType(frequency) {
  if (frequency === 'high' || frequency === '高频') return 'danger'
  if (frequency === 'medium' || frequency === '中频') return 'warning'
  return 'info'
}

function truncateText(text, maximumLength) {
  if (!text) return ''
  return text.length > maximumLength ? `${text.slice(0, maximumLength)}...` : text
}
</script>

<style scoped>
.chapter-detail {
  width: min(100%, 1040px);
}

.chapter-overview {
  display: grid;
  grid-template-columns: 56px 1fr;
  gap: var(--space-4);
  align-items: center;
  padding: var(--space-5);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-xl);
  background: var(--color-surface);
}

.chapter-number {
  display: grid;
  width: 56px;
  height: 56px;
  place-items: center;
  border-radius: var(--radius-lg);
  background: var(--color-primary-light);
  color: var(--color-primary);
  font-size: var(--text-xl);
  font-weight: 800;
}

.overview-label {
  color: var(--color-text-primary);
  font-weight: 700;
}

.chapter-overview p {
  margin-top: var(--space-1);
  color: var(--color-text-secondary);
  line-height: 1.7;
}

.knowledge-preview {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: var(--space-4);
  min-height: 160px;
}

.knowledge-item {
  display: flex;
  min-width: 0;
  flex-direction: column;
  align-items: flex-start;
  padding: var(--space-5);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  background: var(--color-surface);
  transition: border-color var(--motion-fast) var(--ease-out), box-shadow var(--motion-fast) var(--ease-out);
}

.knowledge-item:hover {
  border-color: var(--color-primary-border);
  box-shadow: var(--shadow-sm);
}

.knowledge-heading {
  display: flex;
  width: 100%;
  align-items: flex-start;
  justify-content: space-between;
  gap: var(--space-3);
}

.knowledge-title {
  display: flex;
  min-width: 0;
  align-items: center;
  gap: var(--space-2);
  color: var(--color-text-primary);
}

.knowledge-title .el-icon {
  flex: 0 0 auto;
  color: var(--color-warning);
}

.knowledge-item p {
  flex: 1;
  margin: var(--space-3) 0;
  color: var(--color-text-secondary);
  line-height: 1.7;
}

@media (max-width: 720px) {
  .knowledge-preview {
    grid-template-columns: 1fr;
  }

  .chapter-overview {
    grid-template-columns: 44px 1fr;
    padding: var(--space-4);
  }

  .chapter-number {
    width: 44px;
    height: 44px;
    font-size: var(--text-lg);
  }
}
</style>
