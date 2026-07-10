<template>
  <div class="knowledge-detail">
    <!-- 面包屑 -->
    <el-breadcrumb separator="/" style="margin-bottom: 16px">
      <el-breadcrumb-item :to="{ path: '/courses' }">课程列表</el-breadcrumb-item>
      <el-breadcrumb-item :to="`/chapters/${chapterId}`">章节详情</el-breadcrumb-item>
      <el-breadcrumb-item>重点知识</el-breadcrumb-item>
    </el-breadcrumb>

    <!-- 页面标题 -->
    <div class="page-header">
      <h2>
        <el-icon><Reading /></el-icon>
        重点知识
      </h2>
      <div class="header-actions">
        <el-button @click="$router.push(`/chapters/${chapterId}`)">
          <el-icon><ArrowLeft /></el-icon>
          返回章节
        </el-button>
        <el-button type="primary" @click="$router.push(`/questions?chapterId=${chapterId}`)">
          <el-icon><Edit /></el-icon>
          去题库练习
        </el-button>
      </div>
    </div>

    <!-- 知识点列表 -->
    <div v-loading="loading">
      <div v-if="!loading && knowledgePoints.length === 0">
        <el-empty description="暂无知识点数据" />
      </div>

      <el-card
        v-for="(kp, index) in knowledgePoints"
        :key="kp.id"
        class="kp-card"
        shadow="never"
        :body-style="{ padding: '0' }"
      >
        <!-- 知识点标题栏 -->
        <div class="kp-title-bar">
          <div class="kp-title-left">
            <span class="kp-index">{{ index + 1 }}</span>
            <span class="kp-title-text">{{ kp.title }}</span>
          </div>
          <el-tag
            :type="examFreqType(kp.examFrequency)"
            size="small"
            effect="dark"
          >
            {{ kp.examFrequency || '一般' }}
          </el-tag>
        </div>

        <!-- 知识点主体 -->
        <div class="kp-body">
          <!-- 核心内容 -->
          <div class="kp-section" v-if="kp.coreContent">
            <div class="section-label">
              <el-icon><CircleCheck /></el-icon>
              核心内容
            </div>
            <div class="section-content" v-html="formatContent(kp.coreContent)"></div>
          </div>

          <!-- 常用公式 -->
          <div class="kp-section formula-section" v-if="kp.formulas">
            <div class="section-label">
              <el-icon><Operation /></el-icon>
              常用公式
            </div>
            <div class="section-content" v-html="formatContent(kp.formulas)"></div>
          </div>

          <!-- 高频题型 -->
          <div class="kp-section" v-if="kp.typicalQuestions">
            <div class="section-label">
              <el-icon><TrophyBase /></el-icon>
              高频题型
            </div>
            <div class="section-content" v-html="formatContent(kp.typicalQuestions)"></div>
          </div>

          <!-- 易错点 -->
          <div class="kp-section mistake-section" v-if="kp.commonMistakes">
            <div class="section-label">
              <el-icon><WarningFilled /></el-icon>
              易错点
            </div>
            <div class="section-content" v-html="formatContent(kp.commonMistakes)"></div>
          </div>

          <!-- 复习建议 -->
          <div class="kp-section advice-section" v-if="kp.reviewAdvice">
            <div class="section-label">
              <el-icon><Bell /></el-icon>
              复习建议
            </div>
            <div class="section-content" v-html="formatContent(kp.reviewAdvice)"></div>
          </div>
        </div>
      </el-card>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { getKnowledgePoints } from '../../api/course'
import { ElMessage } from 'element-plus'
import { renderMathContent } from '../../utils/renderMathContent'

const route = useRoute()
const chapterId = route.params.chapterId

const knowledgePoints = ref([])
const loading = ref(false)

onMounted(async () => {
  loading.value = true
  try {
    const res = await getKnowledgePoints(chapterId, { page: 1, pageSize: 100 })
    if (res.code === 200) {
      knowledgePoints.value = res.data?.items || []
    }
  } catch {
    ElMessage.error('加载知识点失败')
  } finally {
    loading.value = false
  }
})

function examFreqType(freq) {
  const map = { high: 'danger', '高频': 'danger', medium: 'warning', '中频': 'warning', low: 'info', '低频': 'info' }
  return map[freq] || 'info'
}

function formatContent(text) {
  return renderMathContent(text)
}
</script>

<style scoped>
.knowledge-detail {
  max-width: 900px;
  margin: 0 auto;
}

/* 页面标题 */
.page-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 24px;
}

.page-header h2 {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 22px;
  color: #303133;
}

.header-actions {
  display: flex;
  gap: 8px;
}

/* 知识点卡片 */
.kp-card {
  margin-bottom: 20px;
  border-radius: 10px;
  overflow: hidden;
  border: 1px solid #e4e7ed;
}

/* 标题栏 */
.kp-title-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 16px 24px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: #fff;
}

.kp-title-left {
  display: flex;
  align-items: center;
  gap: 12px;
}

.kp-index {
  display: inline-flex;
  justify-content: center;
  align-items: center;
  width: 28px;
  height: 28px;
  border-radius: 50%;
  background: rgba(255, 255, 255, 0.25);
  font-size: 14px;
  font-weight: 700;
  flex-shrink: 0;
}

.kp-title-text {
  font-size: 17px;
  font-weight: 600;
}

/* 主体 */
.kp-body {
  padding: 24px;
}

/* 每个区块 */
.kp-section {
  margin-bottom: 20px;
}

.kp-section:last-child {
  margin-bottom: 0;
}

.section-label {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 15px;
  font-weight: 600;
  color: #303133;
  margin-bottom: 10px;
  padding-bottom: 8px;
  border-bottom: 2px solid #409EFF;
}

.section-label .el-icon {
  color: #409EFF;
}

/* 公式区块 */
.formula-section .section-label {
  border-bottom-color: #67c23a;
}

.formula-section .section-label .el-icon {
  color: #67c23a;
}

/* 易错点区块 */
.mistake-section .section-label {
  border-bottom-color: #f56c6c;
}

.mistake-section .section-label .el-icon {
  color: #f56c6c;
}

/* 复习建议区块 */
.advice-section .section-label {
  border-bottom-color: #e6a23c;
}

.advice-section .section-label .el-icon {
  color: #e6a23c;
}

.section-content {
  font-size: 14px;
  color: #4a4a4a;
  line-height: 1.8;
  padding: 4px 0;
}

/* 公式区块特殊样式 */
.formula-section .section-content {
  font-family: 'Courier New', Courier, monospace;
  background: #f0f9eb;
  padding: 12px 16px;
  border-radius: 6px;
  border-left: 3px solid #67c23a;
}

.mistake-section .section-content {
  background: #fef0f0;
  padding: 12px 16px;
  border-radius: 6px;
  border-left: 3px solid #f56c6c;
}

.advice-section .section-content {
  background: #fdf6ec;
  padding: 12px 16px;
  border-radius: 6px;
  border-left: 3px solid #e6a23c;
}
</style>
