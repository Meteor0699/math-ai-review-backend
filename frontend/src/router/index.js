import { createRouter, createWebHistory } from 'vue-router'
import { useUserStore } from '../stores/user'

// 学生端页面
import StudentLayout from '../layouts/StudentLayout.vue'
import Login from '../views/Login.vue'
import Home from '../views/student/Home.vue'
import CourseList from '../views/student/CourseList.vue'
import CourseDetail from '../views/student/CourseDetail.vue'
import ChapterDetail from '../views/student/ChapterDetail.vue'
import KnowledgeDetail from '../views/student/KnowledgeDetail.vue'
import QuestionList from '../views/student/QuestionList.vue'
import QuestionDetail from '../views/student/QuestionDetail.vue'
import PaperList from '../views/student/PaperList.vue'

// 管理员端页面
import AdminLayout from '../layouts/AdminLayout.vue'
import AdminHome from '../views/admin/AdminHome.vue'
import CourseManage from '../views/admin/CourseManage.vue'
import ChapterManage from '../views/admin/ChapterManage.vue'
import KnowledgeManage from '../views/admin/KnowledgeManage.vue'
import QuestionManage from '../views/admin/QuestionManage.vue'
import PaperManage from '../views/admin/PaperManage.vue'
import UserManage from '../views/admin/UserManage.vue'

const routes = [
  {
    path: '/login',
    name: 'Login',
    component: Login,
    meta: { noAuth: true }
  },
  {
    path: '/',
    component: StudentLayout,
    redirect: '/home',
    children: [
      { path: 'home', name: 'Home', component: Home },
      { path: 'courses', name: 'CourseList', component: CourseList },
      { path: 'courses/:courseId', name: 'CourseDetail', component: CourseDetail },
      { path: 'chapters/:chapterId', name: 'ChapterDetail', component: ChapterDetail },
      { path: 'knowledge/:chapterId', name: 'KnowledgeDetail', component: KnowledgeDetail },
      { path: 'questions', name: 'QuestionList', component: QuestionList },
      { path: 'questions/:questionId', name: 'QuestionDetail', component: QuestionDetail },
      { path: 'papers', name: 'PaperList', component: PaperList }
    ]
  },
  {
    path: '/admin',
    component: AdminLayout,
    redirect: '/admin/home',
    meta: { requiresAdmin: true },
    children: [
      { path: 'home', name: 'AdminHome', component: AdminHome },
      { path: 'courses', name: 'CourseManage', component: CourseManage },
      { path: 'chapters', name: 'ChapterManage', component: ChapterManage },
      { path: 'knowledge', name: 'KnowledgeManage', component: KnowledgeManage },
      { path: 'questions', name: 'QuestionManage', component: QuestionManage },
      { path: 'papers', name: 'PaperManage', component: PaperManage },
      { path: 'users', name: 'UserManage', component: UserManage }
    ]
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

// 路由守卫
router.beforeEach((to, from, next) => {
  const token = localStorage.getItem('token')
  const user = JSON.parse(localStorage.getItem('user') || 'null')

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