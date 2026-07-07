<template>
  <el-container class="layout-container">
    <!-- 顶部导航 -->
    <el-header class="layout-header">
      <div class="header-left">
        <h2 class="header-title">
          <el-icon><Setting /></el-icon>
          管理后台
        </h2>
      </div>
      <div class="header-right">
        <el-button text @click="$router.push('/home')">
          <el-icon><View /></el-icon>
          回到前台
        </el-button>
        <el-dropdown>
          <span class="user-info">
            <el-icon><UserFilled /></el-icon>
            {{ userStore.realName || userStore.username }}
            <el-icon><ArrowDown /></el-icon>
          </span>
          <template #dropdown>
            <el-dropdown-item @click="handleLogout">
              <el-icon><SwitchButton /></el-icon>
              退出登录
            </el-dropdown-item>
          </template>
        </el-dropdown>
      </div>
    </el-header>

    <el-container>
      <!-- 侧边菜单 -->
      <el-aside width="220px" class="layout-aside">
        <el-menu
          :default-active="activeMenu"
          router
          background-color="#304156"
          text-color="#bfcbd9"
          active-text-color="#409EFF"
        >
          <el-menu-item index="/admin/home">
            <el-icon><HomeFilled /></el-icon>
            <span>首页</span>
          </el-menu-item>
          <el-menu-item index="/admin/courses">
            <el-icon><Collection /></el-icon>
            <span>课程管理</span>
          </el-menu-item>
          <el-menu-item index="/admin/chapters">
            <el-icon><Folder /></el-icon>
            <span>章节管理</span>
          </el-menu-item>
          <el-menu-item index="/admin/knowledge">
            <el-icon><Reading /></el-icon>
            <span>知识点管理</span>
          </el-menu-item>
          <el-menu-item index="/admin/questions">
            <el-icon><Edit /></el-icon>
            <span>题库管理</span>
          </el-menu-item>
          <el-menu-item index="/admin/papers">
            <el-icon><Document /></el-icon>
            <span>试卷管理</span>
          </el-menu-item>
          <el-menu-item index="/admin/users">
            <el-icon><User /></el-icon>
            <span>用户管理</span>
          </el-menu-item>
        </el-menu>
      </el-aside>

      <!-- 主内容区 -->
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

const activeMenu = computed(() => route.path)

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
  background-color: #2c3e50;
  color: #fff;
  padding: 0 20px;
  height: 60px;
}

.header-title {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 18px;
}

.header-right {
  display: flex;
  align-items: center;
  gap: 12px;
}

.user-info {
  display: flex;
  align-items: center;
  gap: 6px;
  cursor: pointer;
  color: #fff;
  font-size: 14px;
}

.layout-aside {
  background-color: #304156;
  overflow-y: auto;
}

.layout-aside .el-menu {
  border-right: none;
}

.layout-main {
  background-color: #f0f2f5;
  padding: 20px;
  overflow-y: auto;
}
</style>