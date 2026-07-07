<template>
  <div class="admin-home">
    <h2>管理后台</h2>
    <el-divider />
    <el-row :gutter="20">
      <el-col :span="8" v-for="item in menuItems" :key="item.path">
        <el-card shadow="hover" class="menu-card" @click="$router.push(item.path)">
          <el-icon :size="36" :color="item.color">
            <component :is="item.icon" />
          </el-icon>
          <div class="menu-label">{{ item.label }}</div>
          <div class="menu-desc">{{ item.desc }}</div>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup>
const menuItems = [
  { path: '/admin/courses', label: '课程管理', desc: '管理课程信息、编号、描述', icon: 'Collection', color: '#409EFF' },
  { path: '/admin/chapters', label: '章节管理', desc: '管理课程章节和排序', icon: 'Folder', color: '#67c23a' },
  { path: '/admin/knowledge', label: '知识点管理', desc: '管理重点知识、公式、易错点', icon: 'Reading', color: '#e6a23c' },
  { path: '/admin/questions', label: '题库管理', desc: '管理题目、答案、解析', icon: 'Edit', color: '#f56c6c' },
  { path: '/admin/papers', label: '试卷管理', desc: '上传和管理往年试题', icon: 'Document', color: '#909399' },
  { path: '/admin/users', label: '用户管理', desc: '管理用户和权限', icon: 'User', color: '#67c23a' }
]
</script>

<style scoped>
.admin-home {
  max-width: 1000px;
}

.admin-home h2 {
  font-size: 22px;
  color: #303133;
}

.menu-card {
  text-align: center;
  cursor: pointer;
  padding: 24px 16px;
  margin-bottom: 20px;
  border-radius: 10px;
  transition: transform 0.2s, box-shadow 0.2s;
}

.menu-card:hover {
  transform: translateY(-4px);
}

.menu-label {
  font-size: 16px;
  font-weight: 600;
  color: #303133;
  margin-top: 12px;
}

.menu-desc {
  font-size: 13px;
  color: #909399;
  margin-top: 6px;
}
</style>