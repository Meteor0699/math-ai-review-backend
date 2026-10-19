import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import Components from 'unplugin-vue-components/vite'
import { ElementPlusResolver } from 'unplugin-vue-components/resolvers'

export default defineConfig({
  plugins: [
    vue(),
    // Register only the Element Plus components each template actually uses
    // instead of shipping the whole library from main.js.
    Components({ resolvers: [ElementPlusResolver()], dts: false })
  ],
  build: {
    // The entry chunk used to carry Vue, Element Plus, every icon and KaTeX at
    // once (~1.2 MB). Splitting them keeps the vendor code cacheable across
    // app deploys and lets KaTeX load only on pages that render formulas.
    rollupOptions: {
      output: {
        manualChunks: {
          vue: ['vue', 'vue-router', 'pinia'],
          'element-plus-icons': ['@element-plus/icons-vue'],
          katex: ['katex']
        }
      }
    },
    chunkSizeWarningLimit: 800
  },
  server: {
    port: 5173,
    proxy: {
      '/api': {
        target: 'http://localhost:8080',
        changeOrigin: true
      }
    }
  }
})