<template>
  <div class="app-page management-page user-manage">
    <div class="page-header">
      <h2>用户管理</h2>
      <el-button type="primary" @click="openDialog(null)">
        <el-icon><Plus /></el-icon>
        新增用户
      </el-button>
    </div>

    <el-form inline class="filter-bar">
      <el-form-item label="关键词">
        <el-input
          v-model="filters.keyword"
          placeholder="用户名 / 姓名 / 学号"
          clearable
          style="width: 220px"
          @keyup.enter="fetchList"
        />
      </el-form-item>
      <el-form-item label="角色">
        <el-select v-model="filters.role" placeholder="全部" clearable style="width: 130px" @change="fetchList">
          <el-option label="学生" value="student" />
          <el-option label="管理员" value="admin" />
        </el-select>
      </el-form-item>
      <el-form-item>
        <el-button @click="fetchList">查询</el-button>
        <el-button @click="resetFilters">重置</el-button>
      </el-form-item>
    </el-form>

    <el-table :data="tableData" v-loading="loading" border stripe>
      <el-table-column prop="id" label="ID" width="70" />
      <el-table-column prop="username" label="用户名" min-width="130" />
      <el-table-column prop="realName" label="姓名" min-width="120" />
      <el-table-column prop="studentNo" label="学号" min-width="130" />
      <el-table-column label="角色" width="100">
        <template #default="{ row }">
          <el-tag :type="row.role === 'admin' ? 'danger' : 'success'" size="small">
            {{ roleLabel(row.role) }}
          </el-tag>
        </template>
      </el-table-column>
      <el-table-column label="状态" width="90">
        <template #default="{ row }">
          <el-tag :type="row.status === 1 ? 'success' : 'info'" size="small">
            {{ row.status === 1 ? '启用' : '禁用' }}
          </el-tag>
        </template>
      </el-table-column>
      <el-table-column prop="createdAt" label="创建时间" min-width="160" show-overflow-tooltip />
      <el-table-column label="操作" width="240" fixed="right">
        <template #default="{ row }">
          <el-button size="small" text type="primary" @click="openDialog(row)">编辑</el-button>
          <el-button size="small" text type="warning" @click="openPasswordDialog(row)">重置密码</el-button>
          <el-button size="small" text type="danger" :disabled="row.status === 0" @click="handleDisable(row)">
            禁用
          </el-button>
        </template>
      </el-table-column>
    </el-table>

    <el-pagination
      class="pagination"
      layout="total, prev, pager, next"
      :total="total"
      :page-size="pageSize"
      v-model:current-page="page"
      @current-change="fetchList"
    />

    <el-dialog
      v-model="dialogVisible"
      :title="formMode === 'add' ? '新增用户' : '编辑用户'"
      width="520px"
      :close-on-click-modal="false"
    >
      <el-form ref="formRef" :model="form" :rules="rules" label-width="90px">
        <el-form-item label="用户名" prop="username" v-if="formMode === 'add'">
          <el-input v-model="form.username" placeholder="请输入用户名" />
        </el-form-item>
        <el-form-item label="初始密码" prop="password" v-if="formMode === 'add'">
          <el-input v-model="form.password" type="password" show-password placeholder="至少 6 位" />
        </el-form-item>
        <el-form-item label="姓名" prop="realName">
          <el-input v-model="form.realName" placeholder="请输入姓名" />
        </el-form-item>
        <el-form-item label="学号">
          <el-input v-model="form.studentNo" placeholder="管理员可为空" />
        </el-form-item>
        <el-form-item label="角色" prop="role">
          <el-select v-model="form.role" style="width: 100%">
            <el-option label="学生" value="student" />
            <el-option label="管理员" value="admin" />
          </el-select>
        </el-form-item>
        <el-form-item label="状态" prop="status">
          <el-switch v-model="form.status" :active-value="1" :inactive-value="0" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="submitting" @click="handleSubmit">确定</el-button>
      </template>
    </el-dialog>

    <el-dialog v-model="passwordVisible" title="重置密码" width="420px" :close-on-click-modal="false">
      <el-form ref="passwordFormRef" :model="passwordForm" :rules="passwordRules" label-width="90px">
        <el-form-item label="新密码" prop="password">
          <el-input v-model="passwordForm.password" type="password" show-password placeholder="至少 6 位" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="passwordVisible = false">取消</el-button>
        <el-button type="primary" :loading="submitting" @click="handleResetPassword">确定</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { onMounted, reactive, ref } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  adminCreateUser,
  adminDeleteUser,
  adminGetUserList,
  adminResetUserPassword,
  adminUpdateUser
} from '../../api/admin'

const tableData = ref([])
const loading = ref(false)
const submitting = ref(false)
const total = ref(0)
const page = ref(1)
const pageSize = 20

const filters = reactive({ keyword: '', role: '' })
const dialogVisible = ref(false)
const passwordVisible = ref(false)
const formRef = ref(null)
const passwordFormRef = ref(null)
const formMode = ref('add')
const editingId = ref(null)

const form = reactive({
  username: '',
  password: '',
  realName: '',
  studentNo: '',
  role: 'student',
  status: 1
})

const passwordForm = reactive({
  userId: null,
  password: ''
})

const rules = {
  username: [{ required: true, message: '请输入用户名', trigger: 'blur' }],
  password: [{ required: true, min: 6, message: '密码至少 6 位', trigger: 'blur' }],
  realName: [{ required: true, message: '请输入姓名', trigger: 'blur' }],
  role: [{ required: true, message: '请选择角色', trigger: 'change' }]
}

const passwordRules = {
  password: [{ required: true, min: 6, message: '密码至少 6 位', trigger: 'blur' }]
}

onMounted(fetchList)

function roleLabel(role) {
  return role === 'admin' ? '管理员' : '学生'
}

async function fetchList() {
  loading.value = true
  try {
    const res = await adminGetUserList({
      page: page.value,
      pageSize,
      keyword: filters.keyword || undefined,
      role: filters.role || undefined
    })
    if (res.code === 200) {
      tableData.value = res.data?.items || []
      total.value = res.data?.total || 0
    }
  } finally {
    loading.value = false
  }
}

function resetFilters() {
  filters.keyword = ''
  filters.role = ''
  page.value = 1
  fetchList()
}

function openDialog(row) {
  formRef.value?.resetFields()
  if (row) {
    formMode.value = 'edit'
    editingId.value = row.id
    form.username = row.username
    form.password = ''
    form.realName = row.realName || ''
    form.studentNo = row.studentNo || ''
    form.role = row.role
    form.status = row.status
  } else {
    formMode.value = 'add'
    editingId.value = null
    Object.assign(form, {
      username: '',
      password: '',
      realName: '',
      studentNo: '',
      role: 'student',
      status: 1
    })
  }
  dialogVisible.value = true
}

async function handleSubmit() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return

  submitting.value = true
  try {
    const data = {
      realName: form.realName,
      studentNo: form.studentNo,
      role: form.role,
      status: form.status
    }
    let res
    if (formMode.value === 'add') {
      res = await adminCreateUser({ ...data, username: form.username, password: form.password })
    } else {
      res = await adminUpdateUser(editingId.value, data)
    }
    if (res.code === 200) {
      ElMessage.success(formMode.value === 'add' ? '新增成功' : '编辑成功')
      dialogVisible.value = false
      fetchList()
    }
  } finally {
    submitting.value = false
  }
}

function openPasswordDialog(row) {
  passwordFormRef.value?.resetFields()
  passwordForm.userId = row.id
  passwordForm.password = ''
  passwordVisible.value = true
}

async function handleResetPassword() {
  const valid = await passwordFormRef.value.validate().catch(() => false)
  if (!valid) return

  submitting.value = true
  try {
    const res = await adminResetUserPassword(passwordForm.userId, passwordForm.password)
    if (res.code === 200) {
      ElMessage.success('密码已重置')
      passwordVisible.value = false
    }
  } finally {
    submitting.value = false
  }
}

async function handleDisable(row) {
  await ElMessageBox.confirm(`确定禁用用户「${row.username}」吗？`, '禁用确认', { type: 'warning' })
  const res = await adminDeleteUser(row.id)
  if (res.code === 200) {
    ElMessage.success('用户已禁用')
    fetchList()
  }
}
</script>

<style scoped>
.pagination {
  margin-top: var(--space-4);
  justify-content: flex-end;
}
</style>
