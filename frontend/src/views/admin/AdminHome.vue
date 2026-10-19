<template>
  <div class="app-page admin-dashboard">
    <PageHeader title="数据概览" description="查看当前平台真实内容规模，并快速进入管理模块。">
      <template #actions>
        <el-button :icon="Refresh" :loading="loading" @click="loadStats">刷新数据</el-button>
      </template>
    </PageHeader>

    <section v-loading="loading" class="admin-stats">
      <StatCard v-for="item in statItems" :key="item.key" :label="item.label" :value="stats[item.key]" :icon="item.icon" :tone="item.tone" />
    </section>

    <section class="admin-workspace">
      <div class="section-heading">
        <div>
          <h2>内容管理</h2>
          <p>选择模块查看和维护平台数据</p>
        </div>
      </div>
      <div class="management-list">
        <button v-for="item in managementItems" :key="item.path" type="button" @click="$router.push(item.path)">
          <span><el-icon><component :is="item.icon" /></el-icon></span>
          <div>
            <strong>{{ item.label }}</strong>
            <small>{{ item.description }}</small>
          </div>
          <el-icon class="management-arrow"><ArrowRight /></el-icon>
        </button>
      </div>
    </section>
  </div>
</template>

<script setup>
import { onMounted, reactive, ref } from 'vue'
import { Refresh } from '@element-plus/icons-vue'
import PageHeader from '../../components/PageHeader.vue'
import StatCard from '../../components/StatCard.vue'
import { adminGetStats } from '../../api/admin'

const loading = ref(false)
const stats = reactive({
  users: 0,
  courses: 0,
  chapters: 0,
  knowledgePoints: 0,
  questions: 0,
  papers: 0,
  aiExplanations: 0
})

const statItems = [
  { key: 'users', label: '用户数量', icon: 'User', tone: 'primary' },
  { key: 'courses', label: '开放课程', icon: 'Collection', tone: 'success' },
  { key: 'chapters', label: '有效章节', icon: 'Folder', tone: 'warning' },
  { key: 'knowledgePoints', label: '知识点', icon: 'Reading', tone: 'primary' },
  { key: 'questions', label: '题库题目', icon: 'EditPen', tone: 'danger' },
  { key: 'papers', label: '试卷文件', icon: 'Document', tone: 'warning' },
  { key: 'aiExplanations', label: '缓存讲解', icon: 'MagicStick', tone: 'success' }
]

const managementItems = [
  { path: '/admin/courses', label: '课程与章节', description: '维护课程信息和章节学习路径', icon: 'Collection' },
  { path: '/admin/knowledge', label: '重点知识', description: '维护概念、公式、典型题和易错点', icon: 'Reading' },
  { path: '/admin/questions', label: '题库内容', description: '维护题目、选项、答案和普通解析', icon: 'EditPen' },
  { path: '/admin/papers', label: '试卷文件', description: '上传和管理公开或学生试卷', icon: 'Document' },
  { path: '/admin/users', label: '用户与权限', description: '管理学生、管理员和账号状态', icon: 'User' }
]

onMounted(loadStats)

async function loadStats() {
  loading.value = true
  try {
    const response = await adminGetStats()
    Object.assign(stats, response.data || {})
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.admin-stats {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 14px;
  min-height: 104px;
}

.admin-workspace {
  margin-top: 24px;
  padding: 22px;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-xl);
  background: var(--color-surface);
}

.management-list {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}

.management-list button {
  display: grid;
  grid-template-columns: 42px minmax(0, 1fr) 20px;
  align-items: center;
  gap: 12px;
  min-height: 78px;
  padding: 14px;
  border: 0;
  border-radius: var(--radius-lg);
  text-align: left;
  background: var(--color-surface-muted);
  cursor: pointer;
}

.management-list button:hover {
  background: var(--color-primary-light);
}

.management-list button > span {
  display: inline-flex;
  width: 42px;
  height: 42px;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-md);
  color: var(--color-primary);
  background: #fff;
}

.management-list strong,
.management-list small {
  display: block;
}

.management-list small {
  margin-top: 3px;
  color: var(--color-text-secondary);
}

.management-arrow {
  color: var(--color-text-muted);
}

@media (max-width: 1000px) {
  .admin-stats {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 640px) {
  .admin-stats,
  .management-list {
    grid-template-columns: 1fr;
  }

  .admin-workspace {
    padding: 16px;
  }
}
</style>
