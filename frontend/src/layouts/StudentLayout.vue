<template>
  <div class="app-shell" :class="{ 'is-collapsed': collapsed }">
    <aside class="app-sidebar">
      <div class="sidebar-brand">
        <AppLogo :compact="collapsed" />
      </div>
      <div v-if="!collapsed" class="sidebar-label">学习空间</div>
      <el-menu :default-active="activeMenu" router :collapse="collapsed" class="sidebar-menu">
        <el-menu-item v-for="item in navigation" :key="item.path" :index="item.path">
          <el-icon><component :is="item.icon" /></el-icon>
          <template #title>{{ item.label }}</template>
        </el-menu-item>
      </el-menu>
      <button class="collapse-button" type="button" :aria-label="collapsed ? '展开导航' : '收起导航'" @click="collapsed = !collapsed">
        <el-icon><Expand v-if="collapsed" /><Fold v-else /></el-icon>
        <span v-if="!collapsed">收起导航</span>
      </button>
    </aside>

    <header class="app-header">
      <button class="mobile-menu-button" type="button" aria-label="打开导航菜单" @click="drawerOpen = true">
        <el-icon><Menu /></el-icon>
      </button>
      <div class="header-search">
        <el-input
          v-model="searchKeyword"
          :prefix-icon="Search"
          clearable
          placeholder="搜索题目、知识点或课程"
          @keyup.enter="submitSearch"
        />
      </div>
      <div class="header-actions">
        <el-tooltip content="消息中心尚未开放" placement="bottom">
          <button class="icon-button" type="button" aria-label="消息中心" disabled>
            <el-icon><Bell /></el-icon>
          </button>
        </el-tooltip>
        <el-dropdown trigger="click">
          <button class="profile-button" type="button">
            <el-avatar :size="34">{{ avatarText }}</el-avatar>
            <span class="profile-copy">
              <strong>{{ userStore.realName || userStore.username }}</strong>
              <small>{{ userStore.isAdmin ? '管理员' : '学生' }}</small>
            </span>
            <el-icon><ArrowDown /></el-icon>
          </button>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item v-if="userStore.isAdmin" @click="router.push('/admin/home')">
                <el-icon><Setting /></el-icon>管理后台
              </el-dropdown-item>
              <el-dropdown-item divided @click="handleLogout">
                <el-icon><SwitchButton /></el-icon>退出登录
              </el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
      </div>
    </header>

    <main class="app-content">
      <router-view />
    </main>

    <MobileBottomNav :items="mobileNavigation" />

    <el-drawer v-model="drawerOpen" direction="ltr" size="280px" :with-header="false" class="mobile-drawer">
      <div class="drawer-brand"><AppLogo /></div>
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
import { ArrowDown, Bell, Expand, Fold, Menu, Search } from '@element-plus/icons-vue'
import { ElMessageBox } from 'element-plus'
import AppLogo from '../components/AppLogo.vue'
import MobileBottomNav from '../components/MobileBottomNav.vue'
import { useUserStore } from '../stores/user'

const route = useRoute()
const router = useRouter()
const userStore = useUserStore()
const collapsed = ref(false)
const drawerOpen = ref(false)
const searchKeyword = ref('')

const navigation = [
  { path: '/home', label: '首页', icon: 'HomeFilled' },
  { path: '/courses', label: '课程学习', icon: 'Collection' },
  { path: '/questions', label: '题库练习', icon: 'EditPen' },
  { path: '/study', label: '学习记录与错题', icon: 'Notebook' },
  { path: '/papers', label: '往年试题', icon: 'Document' }
]

const mobileNavigation = navigation.map((item) => ({
  ...item,
  label: item.path === '/study' ? '学习中心' : item.label.replace('学习', '')
}))

const activeMenu = computed(() => {
  const path = route.path
  if (path.startsWith('/courses') || path.startsWith('/chapters') || path.startsWith('/knowledge')) return '/courses'
  if (path.startsWith('/questions')) return '/questions'
  if (path.startsWith('/study')) return '/study'
  if (path.startsWith('/papers')) return '/papers'
  return '/home'
})

const avatarText = computed(() => (userStore.realName || userStore.username || '同学').slice(0, 1))

function submitSearch() {
  const keyword = searchKeyword.value.trim()
  router.push({ path: '/questions', query: keyword ? { keyword } : {} })
}

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
.app-shell {
  min-height: 100vh;
  padding-left: var(--sidebar-width);
  background: var(--color-background);
  transition: padding-left var(--motion-base) var(--ease-out);
}

.app-shell.is-collapsed {
  padding-left: var(--sidebar-collapsed-width);
}

.app-sidebar {
  position: fixed;
  z-index: var(--z-header);
  top: 0;
  bottom: 0;
  left: 0;
  display: flex;
  width: var(--sidebar-width);
  flex-direction: column;
  border-right: 1px solid var(--color-border);
  background: var(--color-surface);
  transition: width var(--motion-base) var(--ease-out);
}

.is-collapsed .app-sidebar {
  width: var(--sidebar-collapsed-width);
}

.sidebar-brand {
  display: flex;
  height: var(--header-height);
  align-items: center;
  padding: 0 16px;
  border-bottom: 1px solid var(--color-border);
}

.is-collapsed .sidebar-brand {
  justify-content: center;
  padding: 0;
}

.sidebar-label {
  padding: 22px 24px 8px;
  color: var(--color-text-muted);
  font-size: var(--text-xs);
}

.sidebar-menu {
  flex: 1;
  padding: 8px 12px;
  border: 0;
}

.sidebar-menu :deep(.el-menu-item) {
  height: 44px;
  margin-bottom: 5px;
  border-radius: var(--radius-md);
  color: var(--color-text-secondary);
}

.sidebar-menu :deep(.el-menu-item:hover) {
  color: var(--color-primary);
  background: var(--color-primary-light);
}

.sidebar-menu :deep(.el-menu-item.is-active) {
  color: #fff;
  background: var(--color-primary);
  font-weight: 600;
}

.collapse-button {
  display: flex;
  min-height: 48px;
  align-items: center;
  justify-content: center;
  gap: 8px;
  margin: 12px;
  border: 0;
  border-radius: var(--radius-md);
  color: var(--color-text-secondary);
  background: var(--color-surface-muted);
  cursor: pointer;
}

.app-header {
  position: sticky;
  z-index: calc(var(--z-header) - 1);
  top: 0;
  display: flex;
  height: var(--header-height);
  align-items: center;
  justify-content: space-between;
  gap: 24px;
  padding: 0 28px;
  border-bottom: 1px solid var(--color-border);
  background: rgba(255, 255, 255, 0.96);
}

.header-search {
  width: min(520px, 48vw);
}

.header-actions {
  display: flex;
  align-items: center;
  gap: 10px;
}

.icon-button,
.mobile-menu-button {
  display: inline-flex;
  width: 40px;
  height: 40px;
  align-items: center;
  justify-content: center;
  border: 0;
  border-radius: var(--radius-md);
  color: var(--color-text-secondary);
  background: transparent;
}

.icon-button:disabled {
  cursor: not-allowed;
  opacity: 0.55;
}

.profile-button {
  display: flex;
  min-height: 44px;
  align-items: center;
  gap: 9px;
  padding: 4px 6px;
  border: 0;
  border-radius: var(--radius-md);
  color: var(--color-text-primary);
  background: transparent;
  cursor: pointer;
}

.profile-button:hover {
  background: var(--color-surface-muted);
}

.profile-copy {
  display: flex;
  min-width: 80px;
  max-width: 128px;
  flex-direction: column;
  align-items: flex-start;
}

.profile-copy strong {
  display: block;
  max-width: 100%;
  overflow: hidden;
  font-size: var(--text-sm);
  text-overflow: ellipsis;
  white-space: nowrap;
}

.profile-copy small {
  display: block;
  max-width: 100%;
  overflow: hidden;
  color: var(--color-text-secondary);
  font-size: 11px;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.mobile-menu-button {
  display: none;
}

.app-content {
  min-height: calc(100vh - var(--header-height));
  padding: 28px;
}

.drawer-brand {
  padding: 18px 8px 24px;
  border-bottom: 1px solid var(--color-border);
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
  .app-shell,
  .app-shell.is-collapsed {
    padding-left: 0;
  }

  .app-sidebar {
    display: none;
  }

  .app-header {
    height: 56px;
    gap: 10px;
    padding: 0 14px;
  }

  .mobile-menu-button {
    display: inline-flex;
    flex: 0 0 40px;
  }

  .header-search {
    width: auto;
    flex: 1;
  }

  .icon-button,
  .profile-copy,
  .profile-button > .el-icon {
    display: none;
  }

  .profile-button {
    min-height: 40px;
    padding: 0;
  }

  .app-content {
    min-height: calc(100vh - 56px);
    padding: 18px 14px 88px;
  }
}
</style>
