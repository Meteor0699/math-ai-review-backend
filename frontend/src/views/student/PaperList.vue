<template>
  <div class="paper-list">
    <div class="page-header">
      <h2>往年试题</h2>
    </div>

    <div v-loading="loading">
      <el-card
        v-for="paper in list"
        :key="paper.id"
        class="paper-card"
        shadow="hover"
      >
        <div class="paper-main">
          <div class="paper-info">
            <div class="paper-title">{{ paper.title }}</div>
            <div class="paper-meta">
              <el-tag size="small">{{ paper.examYear || '未知年份' }}</el-tag>
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
  } catch (error) {
    // 错误已在 request 拦截器中处理
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
  max-width: 900px;
  margin: 0 auto;
}

.page-header {
  margin-bottom: 20px;
}

.page-header h2 {
  font-size: 22px;
}

.paper-card {
  margin-bottom: 12px;
}

.paper-main {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.paper-info {
  min-width: 0;
}

.paper-title {
  font-size: 16px;
  font-weight: 600;
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
</style>
