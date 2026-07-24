import { createRouter, createWebHistory } from 'vue-router'
import { readToken, readUser } from '../utils/authStorage'

const StudentLayout = () => import('../layouts/StudentLayout.vue')
const AdminLayout = () => import('../layouts/AdminLayout.vue')

const routes = [
  {
    path: '/login',
    name: 'Login',
    component: () => import('../views/Login.vue'),
    meta: { noAuth: true }
  },
  {
    path: '/',
    component: StudentLayout,
    redirect: '/home',
    children: [
      { path: 'home', name: 'Home', component: () => import('../views/student/Home.vue') },
      { path: 'courses', name: 'CourseList', component: () => import('../views/student/CourseList.vue') },
      { path: 'courses/:courseId', name: 'CourseDetail', component: () => import('../views/student/CourseDetail.vue') },
      { path: 'chapters/:chapterId', name: 'ChapterDetail', component: () => import('../views/student/ChapterDetail.vue') },
      { path: 'knowledge/:chapterId', name: 'KnowledgeDetail', component: () => import('../views/student/KnowledgeDetail.vue') },
      { path: 'questions', name: 'QuestionList', component: () => import('../views/student/QuestionList.vue') },
      { path: 'questions/:questionId', name: 'QuestionDetail', component: () => import('../views/student/QuestionDetail.vue') },
      { path: 'study', name: 'StudyCenter', component: () => import('../views/student/StudyCenter.vue') },
      { path: 'papers', name: 'PaperList', component: () => import('../views/student/PaperList.vue') }
    ]
  },
  {
    path: '/admin',
    component: AdminLayout,
    redirect: '/admin/home',
    meta: { requiresAdmin: true },
    children: [
      { path: 'home', name: 'AdminHome', component: () => import('../views/admin/AdminHome.vue') },
      { path: 'courses', name: 'CourseManage', component: () => import('../views/admin/CourseManage.vue') },
      { path: 'chapters', name: 'ChapterManage', component: () => import('../views/admin/ChapterManage.vue') },
      { path: 'knowledge', name: 'KnowledgeManage', component: () => import('../views/admin/KnowledgeManage.vue') },
      { path: 'questions', name: 'QuestionManage', component: () => import('../views/admin/QuestionManage.vue') },
      { path: 'papers', name: 'PaperManage', component: () => import('../views/admin/PaperManage.vue') },
      { path: 'users', name: 'UserManage', component: () => import('../views/admin/UserManage.vue') }
    ]
  }
]

const router = createRouter({
  history: createWebHistory(),
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

export default router
