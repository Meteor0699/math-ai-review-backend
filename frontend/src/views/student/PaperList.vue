<template>
  <div class="paper-list">
    <div class="page-header">
      <div>
        <h2>往年试题</h2>
        <p>集中查看课程试卷文件，用于考前模拟与题型熟悉</p>
      </div>
    </div>

    <div v-loading="loading" class="paper-stack">
      <el-card
        v-for="paper in list"
        :key="paper.id"
        class="paper-card"
        shadow="hover"
        :body-style="{ padding: '18px 20px' }"
      >
        <div class="paper-main">
          <div class="paper-icon">
            <el-icon><Document /></el-icon>
          </div>
          <div class="paper-info">
            <div class="paper-title">{{ paper.title }}</div>
            <div class="paper-meta">
              <el-tag size="small" effect="plain">{{ paper.examYear || '未知年份' }}</el-tag>
              <span>{{ paper.courseName }}</span>
              <span v-if="paper.originalFilename">{{ paper.originalFilename }}</span>
            </div>
          </div>
          <el-button type="primary" plain size="small" @click="openPaper(paper)">
            查看/下载
          </el-button>
        </div>
      </el-card>

      <el-empty v-if="!loading && list.length === 0" description="暂无往年试题" />
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { getPaperList } from '../../api/course'

const list = ref([])
const loading = ref(false)

onMounted(async () => {
  loading.value = true
  try {
    const res = await getPaperList({ page: 1, pageSize: 20 })
    list.value = res.data?.items || []
  } catch {
    list.value = []
  } finally {
    loading.value = false
  }
})

function openPaper(paper) {
  const url = paper.fileUrl || (paper.id ? `/api/papers/${paper.id}/download` : '')
  if (!url) {
    ElMessage.warning('该试卷暂无文件')
    return
  }
  window.open(url, '_blank')
}
</script>

<style scoped>
.paper-list {
  max-width: 940px;
  margin: 0 auto;
}

.page-header {
  margin-bottom: 18px;
}

.page-header h2 {
  font-size: 22px;
  color: #1f2937;
}

.page-header p {
  color: #909399;
  font-size: 14px;
  margin-top: 6px;
}

.paper-stack {
  display: grid;
  gap: 12px;
}

.paper-card {
  transition: transform 0.2s, border-color 0.2s;
}

.paper-card:hover {
  transform: translateX(4px);
  border-color: #cfe0ff;
}

.paper-main {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.paper-icon {
  width: 42px;
  height: 42px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 8px;
  background: #fff5dd;
  color: #c97706;
  flex-shrink: 0;
}

.paper-info {
  flex: 1;
  min-width: 0;
}

.paper-title {
  font-size: 16px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 8px;
}

.paper-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
  font-size: 12px;
  color: #909399;
}

@media (max-width: 640px) {
  .paper-main {
    align-items: flex-start;
  }
}
</style>
