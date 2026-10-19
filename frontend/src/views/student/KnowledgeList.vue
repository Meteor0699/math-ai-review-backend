<template>
  <div class="knowledge-list">
    <div class="page-header">
      <el-button text @click="$router.back()">
        <el-icon><ArrowLeft /></el-icon> 返回
      </el-button>
      <h2>知识点列表</h2>
    </div>
    <div v-loading="loading">
      <el-card v-for="kp in list" :key="kp.id" class="kp-card" style="margin-bottom: 12px">
        <div class="kp-title">{{ kp.title }}</div>
        <div class="kp-content">{{ kp.coreContent }}</div>
        <div style="margin-top: 8px">
          <el-tag v-if="kp.examFrequency" size="small">{{ kp.examFrequency }}</el-tag>
        </div>
      </el-card>
      <el-empty v-if="!loading && list.length === 0" description="暂无知识点" />
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { getKnowledgePoints } from '../../api/course'

const route = useRoute()
const chapterId = route.params.chapterId
const list = ref([])
const loading = ref(false)

onMounted(async () => {
  loading.value = true
  try {
    const res = await getKnowledgePoints(chapterId, { page: 1, pageSize: 100 })
    list.value = res.data?.items || []
  } catch {
    // 错误已在 request 拦截器中处理
  } finally {
    loading.value = false
  }
})
</script>

<style scoped>
.knowledge-list { max-width: 900px; margin: 0 auto; }
.page-header { display: flex; align-items: center; gap: 12px; margin-bottom: 20px; }
.page-header h2 { font-size: 22px; }
.kp-title { font-size: 16px; font-weight: 600; margin-bottom: 4px; }
.kp-content { font-size: 14px; color: #606266; line-height: 1.6; }
</style>
