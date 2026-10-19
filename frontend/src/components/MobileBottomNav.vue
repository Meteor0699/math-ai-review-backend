<template>
  <nav class="mobile-nav" aria-label="移动端主导航">
    <router-link v-for="item in items" :key="item.path" :to="item.path">
      <el-icon><component :is="item.icon" /></el-icon>
      <span>{{ item.label }}</span>
    </router-link>
  </nav>
</template>

<script setup>
defineProps({
  items: { type: Array, required: true }
})
</script>

<style scoped>
.mobile-nav {
  position: fixed;
  z-index: var(--z-mobile-nav);
  right: 0;
  bottom: 0;
  left: 0;
  display: none;
  min-height: 64px;
  padding: 6px 8px max(6px, env(safe-area-inset-bottom));
  border-top: 1px solid var(--color-border);
  background: var(--color-surface);
}

.mobile-nav a {
  display: flex;
  min-width: 0;
  min-height: 50px;
  flex: 1;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  gap: 3px;
  border-radius: var(--radius-md);
  color: var(--color-text-secondary);
  font-size: 11px;
  text-decoration: none;
}

.mobile-nav a .el-icon {
  font-size: 20px;
}

.mobile-nav a.router-link-active {
  color: var(--color-primary);
  background: var(--color-primary-light);
  font-weight: 600;
}

@media (max-width: 760px) {
  .mobile-nav {
    display: flex;
  }
}
</style>
