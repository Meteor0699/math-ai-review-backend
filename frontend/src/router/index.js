import { createRouter, createWebHistory } from 'vue-router'
import { readToken, readUser } from '../utils/authStorage'

const StudentLayout = () => import('../layouts/StudentLayout.vue')
const AdminLayout = () => import('../layouts/AdminLayout.vue')

const APP_TITLE = '数学 AI 复习助手'

const routes = [
  {
    path: '/login',
    name: 'Login',
    component: () => import('../views/Login.vue'),
    meta: { noAuth: true, title: '登录' }
  },
  {
    path: '/',
    component: StudentLayout,
    redirect: '/home',
    children: [
      {
        path: 'home',
        name: 'Home',
        component: () => import('../views/student/Home.vue'),
        meta: { title: '首页' }
      },
      {
        path: 'courses',
        name: 'CourseList',
        component: () => import('../views/student/CourseList.vue'),
        meta: { title: '课程学习' }
      },
      {
        path: 'courses/:courseId',
        name: 'CourseDetail',
        component: () => import('../views/student/CourseDetail.vue'),
        meta: { title: '课程详情' }
      },
      {
        path: 'chapters/:chapterId',
        name: 'ChapterDetail',
        component: () => import('../views/student/ChapterDetail.vue'),
        meta: { title: '章节详情' }
      },
      {
        path: 'knowledge/:chapterId',
        name: 'KnowledgeDetail',
        component: () => import('../views/student/KnowledgeDetail.vue'),
        meta: { title: '重点知识' }
      },
      {
        path: 'questions',
        name: 'QuestionList',
        component: () => import('../views/student/QuestionList.vue'),
        meta: { title: '题库练习' }
      },
      {
        path: 'questions/:questionId',
        name: 'QuestionDetail',
        component: () => import('../views/student/QuestionDetail.vue'),
        meta: { title: '题目详情' }
      },
      {
        path: 'study',
        name: 'StudyCenter',
        component: () => import('../views/student/StudyCenter.vue'),
        meta: { title: '学习中心' }
      },
      {
        path: 'papers',
        name: 'PaperList',
        component: () => import('../views/student/PaperList.vue'),
        meta: { title: '往年试题' }
      }
    ]
  },
  {
    path: '/admin',
    component: AdminLayout,
    redirect: '/admin/home',
    meta: { requiresAdmin: true },
    children: [
      {
        path: 'home',
        name: 'AdminHome',
        component: () => import('../views/admin/AdminHome.vue'),
        meta: { title: '数据概览' }
      },
      {
        path: 'courses',
        name: 'CourseManage',
        component: () => import('../views/admin/CourseManage.vue'),
        meta: { title: '课程管理' }
      },
      {
        path: 'chapters',
        name: 'ChapterManage',
        component: () => import('../views/admin/ChapterManage.vue'),
        meta: { title: '章节管理' }
      },
      {
        path: 'knowledge',
        name: 'KnowledgeManage',
        component: () => import('../views/admin/KnowledgeManage.vue'),
        meta: { title: '知识点管理' }
      },
      {
        path: 'questions',
        name: 'QuestionManage',
        component: () => import('../views/admin/QuestionManage.vue'),
        meta: { title: '题库管理' }
      },
      {
        path: 'papers',
        name: 'PaperManage',
        component: () => import('../views/admin/PaperManage.vue'),
        meta: { title: '试卷管理' }
      },
      {
        path: 'users',
        name: 'UserManage',
        component: () => import('../views/admin/UserManage.vue'),
        meta: { title: '用户管理' }
      }
    ]
  }
]

const router = createRouter({
  history: createWebHistory(),
  scrollBehavior: () => ({ top: 0 }),
  routes
})

// 路由守卫
router.beforeEach((to, from, next) => {
  const token = readToken()
  const user = readUser()

  // 不需要登录的页面
  if (to.meta.noAuth) {
    if (token && to.path === '/login') {
      next('/home')
      return
    }
    next()
    return
  }

  // 未登录，跳转登录页
  if (!token) {
    next('/login')
    return
  }

  // 需要管理员权限
  if (to.meta.requiresAdmin || to.path.startsWith('/admin')) {
    if (user?.role !== 'admin') {
      next('/home')
      return
    }
  }

  next()
})

// 单页应用不会自动更新标题或移动焦点：两者都在这里补齐，屏幕阅读器
// 才能在导航后获知新页面。焦点目标由两个布局的 <main id="main-content"> 提供。
router.afterEach((to) => {
  document.title = to.meta.title ? `${to.meta.title} · ${APP_TITLE}` : APP_TITLE
  const main = document.getElementById('main-content')
  if (main) main.focus()
})

export default router
