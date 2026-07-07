<template>
  <el-container class="layout-container">
    <!-- 顶部导航 -->
    <el-header class="layout-header">
      <div class="header-left">
        <router-link to="/home" class="header-logo">
          <el-icon :size="22"><School /></el-icon>
          <span class="logo-text">数学 AI 复习助手</span>
        </router-link>
      </div>
      <div class="header-right">
        <el-dropdown trigger="click">
          <span class="user-info">
            <el-avatar :size="32" icon="UserFilled" />
            <span class="user-name">{{ userStore.realName || userStore.username }}</span>
          </span>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item v-if="userStore.isAdmin" @click="$router.push('/admin/home')">
                <el-icon><Setting /></el-icon>
                管理后台
              </el-dropdown-item>
              <el-dropdown-item divided @click="handleLogout">
                <el-icon><SwitchButton /></el-icon>
                退出登录
              </el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
      </div>
    </el-header>

    <el-container>
      <!-- 侧边导航 -->
      <el-aside width="200px" class="layout-aside">
        <el-menu
          :default-active="activeMenu"
          router
          background-color="#fff"
          text-color="#606266"
          active-text-color="#409EFF"
          style="border-right: none"
        >
          <el-menu-item index="/home">
            <el-icon><HomeFilled /></el-icon>
            <span>首页</span>
          </el-menu-item>
          <el-menu-item index="/courses">
            <el-icon><Collection /></el-icon>
            <span>课程列表</span>
          </el-menu-item>
          <el-menu-item index="/questions">
            <el-icon><Edit /></el-icon>
            <span>题库</span>
          </el-menu-item>
          <el-menu-item index="/papers">
            <el-icon><Document /></el-icon>
            <span>往年试题</span>
          </el-menu-item>
        </el-menu>
      </el-aside>

      <!-- 主内容 -->
      <el-main class="layout-main">
        <router-view />
      </el-main>
    </el-container>
  </el-container>
</template>

<script setup>
import { computed } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useUserStore } from '../stores/user'
import { ElMessageBox } from 'element-plus'

const route = useRoute()
const router = useRouter()
const userStore = useUserStore()

const activeMenu = computed(() => {
  const path = route.path
  if (path.startsWith('/courses')) return '/courses'
  if (path.startsWith('/questions')) return '/questions'
  if (path.startsWith('/papers')) return '/papers'
  if (path.startsWith('/chapters') || path.startsWith('/knowledge')) return '/courses'
  return path
})

function handleLogout() {
  ElMessageBox.confirm('确定要退出登录吗？', '提示', {
    confirmButtonText: '确定',
    cancelButtonText: '取消',
    type: 'warning'
  }).then(() => {
    userStore.logout()
    router.push('/login')
  }).catch(() => {})
}
</script>

<style scoped>
.layout-container {
  height: 100vh;
}

.layout-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;
  padding: 0 24px;
  height: 56px;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.04);
}

.header-logo {
  display: flex;
  align-items: center;
  gap: 8px;
  text-decoration: none;
  color: #303133;
}

.logo-text {
  font-size: 17px;
  font-weight: 600;
}

.header-right {
  display: flex;
  align-items: center;
}

.user-info {
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
}

.user-name {
  font-size: 14px;
  color: #606266;
}

.layout-aside {
  background: #fff;
  border-right: 1px solid #e4e7ed;
  overflow-y: auto;
}

.layout-aside .el-menu {
  padding-top: 8px;
}

.layout-main {
  background-color: #f5f7fa;
  padding: 24px;
  overflow-y: auto;
}
</style>