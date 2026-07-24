<template>
  <div class="admin-shell">
    <aside class="admin-sidebar">
      <div class="admin-brand"><AppLogo to="/admin/home" /></div>
      <div class="admin-label">管理工作台</div>
      <el-menu :default-active="activeMenu" router class="admin-menu">
        <el-menu-item v-for="item in navigation" :key="item.path" :index="item.path">
          <el-icon><component :is="item.icon" /></el-icon>
          <span>{{ item.label }}</span>
        </el-menu-item>
      </el-menu>
    </aside>

    <header class="admin-header">
      <button class="menu-button" type="button" aria-label="打开管理导航" @click="drawerOpen = true">
        <el-icon><Menu /></el-icon>
      </button>
      <div>
        <strong>{{ currentTitle }}</strong>
        <span>管理后台</span>
      </div>
      <div class="admin-actions">
        <el-button text @click="router.push('/home')">
          <el-icon><Back /></el-icon>返回学生端
        </el-button>
        <el-dropdown trigger="click">
          <button class="admin-profile" type="button">
            <el-avatar :size="34">{{ avatarText }}</el-avatar>
            <span>{{ userStore.realName || userStore.username }}</span>
            <el-icon><ArrowDown /></el-icon>
          </button>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item @click="handleLogout">
                <el-icon><SwitchButton /></el-icon>退出登录
              </el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
      </div>
    </header>

    <main class="admin-content"><router-view /></main>

    <el-drawer v-model="drawerOpen" direction="ltr" size="280px" :with-header="false">
      <div class="admin-brand"><AppLogo to="/admin/home" /></div>
      <nav class="drawer-navigation">
        <router-link v-for="item in navigation" :key="item.path" :to="item.path" @click="drawerOpen = false">
          <el-icon><component :is="item.icon" /></el-icon>
          <span>{{ item.label }}</span>
        </router-link>
      </nav>
    </el-drawer>
  </div>
</template>

<script setup>
import { computed, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ArrowDown, Back, Menu } from '@element-plus/icons-vue'
import { ElMessageBox } from 'element-plus'
import AppLogo from '../components/AppLogo.vue'
import { useUserStore } from '../stores/user'

const route = useRoute()
const router = useRouter()
const userStore = useUserStore()
const drawerOpen = ref(false)

const navigation = [
  { path: '/admin/home', label: '数据概览', icon: 'DataBoard' },
  { path: '/admin/courses', label: '课程管理', icon: 'Collection' },
  { path: '/admin/chapters', label: '章节管理', icon: 'Folder' },
  { path: '/admin/knowledge', label: '知识点管理', icon: 'Reading' },
  { path: '/admin/questions', label: '题库管理', icon: 'EditPen' },
  { path: '/admin/papers', label: '试卷管理', icon: 'Document' },
  { path: '/admin/users', label: '用户管理', icon: 'User' }
]

const activeMenu = computed(() => route.path)
const currentTitle = computed(() => navigation.find((item) => route.path.startsWith(item.path))?.label || '管理后台')
const avatarText = computed(() => (userStore.realName || userStore.username || '管').slice(0, 1))

function handleLogout() {
  ElMessageBox.confirm('确定退出当前账号吗？', '退出登录', {
    confirmButtonText: '退出',
    cancelButtonText: '取消',
    type: 'warning'
  }).then(() => {
    userStore.logout()
    router.push('/login')
  }).catch(() => {})
}
</script>

<style scoped>
.admin-shell {
  min-height: 100vh;
  padding-left: var(--sidebar-width);
  background: var(--color-background);
}

.admin-sidebar {
  position: fixed;
  z-index: var(--z-header);
  top: 0;
  bottom: 0;
  left: 0;
  width: var(--sidebar-width);
  border-right: 1px solid var(--color-border);
  background: var(--color-surface);
}

.admin-brand {
  display: flex;
  height: var(--header-height);
  align-items: center;
  padding: 0 16px;
  border-bottom: 1px solid var(--color-border);
}

.admin-label {
  padding: 22px 24px 8px;
  color: var(--color-text-muted);
  font-size: var(--text-xs);
}

.admin-menu {
  padding: 8px 12px;
  border: 0;
}

.admin-menu :deep(.el-menu-item) {
  height: 44px;
  margin-bottom: 5px;
  border-radius: var(--radius-md);
  color: var(--color-text-secondary);
}

.admin-menu :deep(.el-menu-item.is-active) {
  color: #fff;
  background: var(--color-primary);
  font-weight: 600;
}

.admin-header {
  position: sticky;
  z-index: calc(var(--z-header) - 1);
  top: 0;
  display: flex;
  height: var(--header-height);
  align-items: center;
  justify-content: space-between;
  padding: 0 28px;
  border-bottom: 1px solid var(--color-border);
  background: rgba(255, 255, 255, 0.96);
}

.admin-header > div:first-of-type {
  display: flex;
  flex-direction: column;
}

.admin-header strong {
  font-size: var(--text-md);
}

.admin-header span {
  color: var(--color-text-secondary);
  font-size: var(--text-xs);
}

.admin-actions {
  display: flex;
  align-items: center;
  gap: 12px;
}

.admin-profile {
  display: flex;
  min-height: 44px;
  align-items: center;
  gap: 8px;
  padding: 4px 6px;
  border: 0;
  border-radius: var(--radius-md);
  background: transparent;
  cursor: pointer;
}

.admin-content {
  min-height: calc(100vh - var(--header-height));
  padding: 28px;
}

.menu-button {
  display: none;
  width: 40px;
  height: 40px;
  align-items: center;
  justify-content: center;
  border: 0;
  border-radius: var(--radius-md);
  background: transparent;
}

.drawer-navigation {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 16px 0;
}

.drawer-navigation a {
  display: flex;
  min-height: 46px;
  align-items: center;
  gap: 12px;
  padding: 0 14px;
  border-radius: var(--radius-md);
  color: var(--color-text-secondary);
  text-decoration: none;
}

.drawer-navigation a.router-link-active {
  color: var(--color-primary);
  background: var(--color-primary-light);
  font-weight: 600;
}

@media (max-width: 760px) {
  .admin-shell {
    padding-left: 0;
  }

  .admin-sidebar {
    display: none;
  }

  .admin-header {
    height: 56px;
    gap: 10px;
    padding: 0 14px;
  }

  .menu-button {
    display: inline-flex;
  }

  .admin-header > div:first-of-type {
    flex: 1;
  }

  .admin-actions .el-button,
  .admin-profile > span,
  .admin-profile > .el-icon {
    display: none;
  }

  .admin-content {
    padding: 18px 14px 32px;
  }
}
</style>
