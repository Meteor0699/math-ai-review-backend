<template>
  <el-container class="layout-container">
    <el-header class="layout-header">
      <div class="header-left">
        <router-link to="/home" class="header-logo">
          <span class="logo-mark">
            <el-icon :size="22"><School /></el-icon>
          </span>
          <span class="logo-text">数研 AI</span>
          <span class="logo-sub">大学数学复习</span>
        </router-link>
      </div>
      <div class="header-right">
        <span class="study-pill">期末复习模式</span>
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
      <el-aside width="216px" class="layout-aside">
        <div class="aside-title">学习导航</div>
        <el-menu
          :default-active="activeMenu"
          router
          background-color="transparent"
          text-color="#4b5563"
          active-text-color="#2563eb"
          style="border-right: none"
        >
          <el-menu-item index="/home">
            <el-icon><HomeFilled /></el-icon>
            <span>今日学习</span>
          </el-menu-item>
          <el-menu-item index="/courses">
            <el-icon><Collection /></el-icon>
            <span>课程与教材</span>
          </el-menu-item>
          <el-menu-item index="/questions">
            <el-icon><Edit /></el-icon>
            <span>题库练习</span>
          </el-menu-item>
          <el-menu-item index="/study">
            <el-icon><Notebook /></el-icon>
            <span>学习记录</span>
          </el-menu-item>
          <el-menu-item index="/papers">
            <el-icon><Document /></el-icon>
            <span>往年试题</span>
          </el-menu-item>
        </el-menu>
      </el-aside>

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
  if (path.startsWith('/study')) return '/study'
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
  background: var(--study-bg);
}

.layout-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background: rgba(255, 255, 255, 0.96);
  border-bottom: 1px solid var(--study-border);
  padding: 0 28px;
  height: 60px;
  backdrop-filter: blur(10px);
}

.header-logo {
  display: flex;
  align-items: center;
  gap: 10px;
  text-decoration: none;
  color: var(--study-text);
}

.logo-mark {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 36px;
  height: 36px;
  border-radius: 8px;
  background: #eaf2ff;
  color: var(--study-primary);
}

.logo-text {
  font-size: 18px;
  font-weight: 700;
}

.logo-sub {
  color: var(--study-muted);
  font-size: 13px;
  padding-left: 2px;
}

.header-right {
  display: flex;
  align-items: center;
  gap: 18px;
}

.study-pill {
  font-size: 12px;
  color: #2563eb;
  background: #eaf2ff;
  border: 1px solid #cfe0ff;
  border-radius: 999px;
  padding: 6px 10px;
}

.user-info {
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
}

.user-name {
  font-size: 14px;
  color: #374151;
}

.layout-aside {
  background: #fff;
  border-right: 1px solid var(--study-border);
  overflow-y: auto;
  padding: 18px 12px;
}

.aside-title {
  font-size: 12px;
  color: #9ca3af;
  padding: 0 14px 10px;
}

.layout-aside :deep(.el-menu-item) {
  height: 44px;
  border-radius: 8px;
  margin-bottom: 6px;
}

.layout-aside :deep(.el-menu-item.is-active) {
  background: var(--study-primary-soft);
  font-weight: 600;
}

.layout-main {
  background:
    linear-gradient(180deg, rgba(234, 242, 255, 0.55), rgba(246, 248, 251, 0) 260px),
    var(--study-bg);
  padding: 28px;
  overflow-y: auto;
}

@media (max-width: 760px) {
  .layout-aside {
    display: none;
  }

  .layout-main {
    padding: 18px;
  }

  .logo-sub,
  .study-pill {
    display: none;
  }
}
</style>
