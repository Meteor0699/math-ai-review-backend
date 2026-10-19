import { createApp } from 'vue'
import { createPinia } from 'pinia'
// Element Plus components are imported on demand by unplugin-vue-components,
// which also injects each component's stylesheet. Only the imperative APIs
// (message / message-box) need their styles imported explicitly, and the
// loading directive is registered by hand.
import 'element-plus/es/components/message/style/css'
import 'element-plus/es/components/message-box/style/css'
import { ElLoading } from 'element-plus'
import * as ElementPlusIconsVue from '@element-plus/icons-vue'
import App from './App.vue'
import router from './router'
import './styles/design-system.css'

const app = createApp(App)

// 注册所有 Element Plus 图标
for (const [key, component] of Object.entries(ElementPlusIconsVue)) {
  app.component(key, component)
}

// v-loading is a directive, not a component, so the resolver cannot add it.
if (ElLoading && ElLoading.directive) {
  app.directive('loading', ElLoading.directive)
}

app.use(createPinia())
app.use(router)

app.mount('#app')
