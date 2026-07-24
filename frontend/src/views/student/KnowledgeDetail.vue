<template>
  <div class="app-page knowledge-page">
    <PageHeader title="重点知识" description="按知识点复习核心概念、公式、典型题和常见错误。">
      <template #breadcrumb>
        <el-breadcrumb separator="/">
          <el-breadcrumb-item :to="{ path: '/courses' }">课程学习</el-breadcrumb-item>
          <el-breadcrumb-item :to="`/chapters/${chapterId}`">章节详情</el-breadcrumb-item>
          <el-breadcrumb-item>重点知识</el-breadcrumb-item>
        </el-breadcrumb>
      </template>
      <template #actions>
        <el-button @click="$router.push(`/chapters/${chapterId}`)">返回章节</el-button>
        <el-button type="primary" @click="$router.push(`/questions?chapterId=${chapterId}`)">章节题库</el-button>
      </template>
    </PageHeader>

    <div v-loading="loading" class="knowledge-layout">
      <aside v-if="knowledgePoints.length" class="knowledge-nav">
        <span>本章知识点</span>
        <button
          v-for="(point, index) in knowledgePoints"
          :key="point.id"
          type="button"
          :class="{ active: activeId === point.id }"
          @click="scrollToPoint(point.id)"
        >
          <strong>{{ index + 1 }}</strong>
          <span>{{ point.title }}</span>
        </button>
      </aside>

      <main class="knowledge-content">
        <article
          v-for="(point, index) in knowledgePoints"
          :id="`knowledge-${point.id}`"
          :key="point.id"
          class="knowledge-article"
        >
          <header>
            <div>
              <span>知识点 {{ index + 1 }}</span>
              <h2>{{ point.title }}</h2>
            </div>
            <el-tag :type="examFrequencyType(point.examFrequency)" effect="plain">
              {{ examFrequencyLabel(point.examFrequency) }}
            </el-tag>
          </header>

          <section v-if="point.coreContent">
            <h3><el-icon><CircleCheck /></el-icon>核心概念</h3>
            <FormulaContent :content="point.coreContent" />
          </section>
          <section v-if="point.formulas" class="formula-block">
            <h3><el-icon><Operation /></el-icon>常用公式</h3>
            <FormulaContent :content="point.formulas" />
          </section>
          <section v-if="point.typicalQuestions">
            <h3><el-icon><TrophyBase /></el-icon>典型题型</h3>
            <FormulaContent :content="point.typicalQuestions" />
          </section>
          <div class="knowledge-notes">
            <section v-if="point.commonMistakes" class="mistake-block">
              <h3><el-icon><WarningFilled /></el-icon>易错点</h3>
              <FormulaContent :content="point.commonMistakes" />
            </section>
            <section v-if="point.reviewAdvice" class="advice-block">
              <h3><el-icon><Bell /></el-icon>复习建议</h3>
              <FormulaContent :content="point.reviewAdvice" />
            </section>
          </div>
          <footer>
            <el-button type="primary" plain @click="$router.push(`/questions?chapterId=${chapterId}`)">练习关联题目</el-button>
          </footer>
        </article>
        <el-empty v-if="!loading && !knowledgePoints.length" description="本章暂未录入重点知识" />
      </main>
    </div>
  </div>
</template>

<script setup>
import { onMounted, ref } from 'vue'
import { useRoute } from 'vue-router'
import { ElMessage } from 'element-plus'
import FormulaContent from '../../components/FormulaContent.vue'
import PageHeader from '../../components/PageHeader.vue'
import { getKnowledgePoints } from '../../api/course'

const route = useRoute()
const chapterId = route.params.chapterId
const knowledgePoints = ref([])
const loading = ref(false)
const activeId = ref(null)

onMounted(async () => {
  loading.value = true
  try {
    const response = await getKnowledgePoints(chapterId, { page: 1, pageSize: 100 })
    knowledgePoints.value = response.data?.items || []
    activeId.value = knowledgePoints.value[0]?.id || null
  } catch {
    ElMessage.error('加载知识点失败')
  } finally {
    loading.value = false
  }
})

function scrollToPoint(id) {
  activeId.value = id
  document.getElementById(`knowledge-${id}`)?.scrollIntoView({ behavior: 'smooth', block: 'start' })
}

function examFrequencyLabel(value) {
  return { high: '高频考点', medium: '中频考点', low: '基础考点' }[value] || value || '一般'
}

function examFrequencyType(value) {
  return { high: 'danger', medium: 'warning', low: 'info', 高频: 'danger', 中频: 'warning', 低频: 'info' }[value] || 'info'
}
</script>

<style scoped>
.knowledge-layout {
  display: grid;
  grid-template-columns: 240px minmax(0, 1fr);
  align-items: start;
  gap: 20px;
  min-height: 360px;
}

.knowledge-nav {
  position: sticky;
  top: calc(var(--header-height) + 20px);
  display: grid;
  gap: 6px;
  padding: 16px;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  background: var(--color-surface);
}

.knowledge-nav > span {
  padding: 4px 8px 10px;
  color: var(--color-text-secondary);
  font-size: var(--text-xs);
}

.knowledge-nav button {
  display: grid;
  grid-template-columns: 28px minmax(0, 1fr);
  align-items: center;
  gap: 8px;
  min-height: 44px;
  padding: 7px 8px;
  border: 0;
  border-radius: var(--radius-md);
  color: var(--color-text-secondary);
  text-align: left;
  background: transparent;
  cursor: pointer;
}

.knowledge-nav button:hover,
.knowledge-nav button.active {
  color: var(--color-primary);
  background: var(--color-primary-light);
}

.knowledge-nav strong {
  display: inline-flex;
  width: 26px;
  height: 26px;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-sm);
  color: inherit;
  background: #fff;
}

.knowledge-nav button > span {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.knowledge-content {
  display: grid;
  gap: 18px;
}

.knowledge-article {
  scroll-margin-top: calc(var(--header-height) + 20px);
  padding: 26px;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-xl);
  background: var(--color-surface);
}

.knowledge-article > header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 14px;
  padding-bottom: 18px;
  border-bottom: 1px solid var(--color-border);
}

.knowledge-article > header span {
  color: var(--color-primary);
  font-size: var(--text-xs);
}

.knowledge-article > header h2 {
  margin-top: 3px;
  font-size: var(--text-lg);
}

.knowledge-article > section {
  padding: 20px 0 4px;
}

.knowledge-article h3 {
  display: flex;
  align-items: center;
  gap: 7px;
  margin-bottom: 10px;
  font-size: var(--text-base);
}

.knowledge-article h3 .el-icon {
  color: var(--color-primary);
}

.formula-block {
  margin-top: 16px;
  padding: 18px !important;
  border-radius: var(--radius-lg);
  background: var(--color-primary-light);
}

.knowledge-notes {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
  margin-top: 20px;
}

.knowledge-notes section {
  padding: 16px;
  border-radius: var(--radius-lg);
}

.mistake-block {
  background: var(--color-danger-light);
}

.mistake-block h3 .el-icon {
  color: var(--color-danger);
}

.advice-block {
  background: var(--color-warning-light);
}

.advice-block h3 .el-icon {
  color: var(--color-warning);
}

.knowledge-article footer {
  display: flex;
  justify-content: flex-end;
  margin-top: 20px;
  padding-top: 16px;
  border-top: 1px solid var(--color-border);
}

@media (max-width: 860px) {
  .knowledge-layout {
    grid-template-columns: 1fr;
  }

  .knowledge-nav {
    position: static;
    display: flex;
    overflow-x: auto;
  }

  .knowledge-nav > span {
    display: none;
  }

  .knowledge-nav button {
    flex: 0 0 auto;
  }
}

@media (max-width: 640px) {
  .knowledge-article {
    padding: 18px 16px;
  }

  .knowledge-notes {
    grid-template-columns: 1fr;
  }
}
</style>
