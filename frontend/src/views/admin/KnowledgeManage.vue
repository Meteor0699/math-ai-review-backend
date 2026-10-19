<template>
  <div class="app-page management-page knowledge-manage">
    <div class="page-header">
      <h2>知识点管理</h2>
      <el-button type="primary" @click="openDialog(null)">
        <el-icon><Plus /></el-icon>
        新增知识点
      </el-button>
    </div>

    <el-form inline class="filter-bar">
      <el-form-item label="所属课程">
        <el-select v-model="filterCourseId" placeholder="全部课程" clearable @change="onCourseFilter">
          <el-option v-for="c in courseList" :key="c.id" :label="c.courseName" :value="c.id" />
        </el-select>
      </el-form-item>
      <el-form-item label="所属章节">
        <el-select v-model="filterChapterId" placeholder="全部章节" clearable @change="fetchList">
          <el-option v-for="ch in chapterList" :key="ch.id" :label="ch.title" :value="ch.id" />
        </el-select>
      </el-form-item>
    </el-form>

    <el-table :data="tableData" v-loading="loading" border stripe>
      <el-table-column prop="id" label="ID" width="60" />
      <el-table-column prop="title" label="知识点标题" min-width="180" show-overflow-tooltip />
      <el-table-column prop="courseName" label="所属课程" width="120" />
      <el-table-column prop="chapterName" label="所属章节" width="120" />
      <el-table-column prop="examFrequency" label="考核频率" width="100">
        <template #default="{ row }">
          <el-tag :type="freqTagType(row.examFrequency)" size="small">{{ row.examFrequency || '一般' }}</el-tag>
        </template>
      </el-table-column>
      <el-table-column prop="sortOrder" label="排序" width="70" />
      <el-table-column label="操作" width="160" fixed="right">
        <template #default="{ row }">
          <el-button size="small" text type="primary" @click="openDialog(row)">编辑</el-button>
          <el-button size="small" text type="danger" @click="handleDelete(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <el-dialog
      v-model="dialogVisible"
      :title="formMode === 'add' ? '新增知识点' : '编辑知识点'"
      width="650px"
      :close-on-click-modal="false"
    >
      <el-form ref="formRef" :model="form" :rules="rules" label-width="80px">
        <el-form-item label="所属课程" prop="courseId">
          <el-select v-model="form.courseId" placeholder="请选择课程" style="width: 100%" @change="onFormCourseChange">
            <el-option v-for="c in courseList" :key="c.id" :label="c.courseName" :value="c.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="所属章节" prop="chapterId">
          <el-select v-model="form.chapterId" placeholder="请选择章节" style="width: 100%">
            <el-option v-for="ch in formChapterList" :key="ch.id" :label="ch.title" :value="ch.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="知识点标题" prop="title">
          <el-input v-model="form.title" placeholder="请输入知识点标题" />
        </el-form-item>
        <el-form-item label="考核频率">
          <el-select v-model="form.examFrequency" style="width: 100%">
            <el-option label="高频" value="high" />
            <el-option label="中频" value="medium" />
            <el-option label="低频" value="low" />
          </el-select>
        </el-form-item>
        <el-form-item label="核心内容">
          <el-input v-model="form.coreContent" type="textarea" :rows="3" placeholder="请输入核心内容" />
        </el-form-item>
        <el-form-item label="常用公式">
          <el-input v-model="form.formulas" type="textarea" :rows="3" placeholder="请输入常用公式" />
        </el-form-item>
        <el-form-item label="高频题型">
          <el-input v-model="form.typicalQuestions" type="textarea" :rows="3" placeholder="请输入高频题型" />
        </el-form-item>
        <el-form-item label="易错点">
          <el-input v-model="form.commonMistakes" type="textarea" :rows="3" placeholder="请输入易错点" />
        </el-form-item>
        <el-form-item label="复习建议">
          <el-input v-model="form.reviewAdvice" type="textarea" :rows="2" placeholder="请输入复习建议" />
        </el-form-item>
        <el-form-item label="排序">
          <el-input-number v-model="form.sortOrder" :min="0" :max="999" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="submitting" @click="handleSubmit">确定</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted, reactive } from 'vue'
import {
  adminGetKnowledgeList, adminCreateKnowledge, adminUpdateKnowledge, adminDeleteKnowledge,
  adminGetCourseList, adminGetChapterList
} from '../../api/admin'
import { ElMessage, ElMessageBox } from 'element-plus'

const tableData = ref([])
const courseList = ref([])
const chapterList = ref([])
const formChapterList = ref([])
const loading = ref(false)
const dialogVisible = ref(false)
const formMode = ref('add')
const editingId = ref(null)
const submitting = ref(false)
const filterCourseId = ref(null)
const filterChapterId = ref(null)
const formRef = ref(null)

const form = reactive({
  courseId: null, chapterId: null, title: '', examFrequency: '',
  coreContent: '', formulas: '', typicalQuestions: '', commonMistakes: '', reviewAdvice: '', sortOrder: 0
})
const rules = {
  courseId: [{ required: true, message: '请选择课程', trigger: 'change' }],
  chapterId: [{ required: true, message: '请选择章节', trigger: 'change' }],
  title: [{ required: true, message: '请输入知识点标题', trigger: 'blur' }]
}

onMounted(async () => {
  await loadCourses()
  fetchList()
})

async function loadCourses() {
  try {
    const res = await adminGetCourseList({ page: 1, pageSize: 100 })
    if (res.code === 200) courseList.value = res.data?.items || []
  } catch { /* 忽略 */ }
}

async function loadChapters(courseId) {
  try {
    const res = await adminGetChapterList({ courseId, page: 1, pageSize: 100 })
    if (res.code === 200) return res.data?.items || []
  } catch { return [] }
  return []
}

async function onCourseFilter() {
  filterChapterId.value = null
  chapterList.value = filterCourseId.value ? await loadChapters(filterCourseId.value) : []
  fetchList()
}

async function onFormCourseChange() {
  form.chapterId = null
  formChapterList.value = form.courseId ? await loadChapters(form.courseId) : []
}

async function fetchList() {
  loading.value = true
  try {
    const params = { page: 1, pageSize: 100 }
    if (filterChapterId.value) params.chapterId = filterChapterId.value
    else if (filterCourseId.value) params.courseId = filterCourseId.value
    const res = await adminGetKnowledgeList(params)
    if (res.code === 200) tableData.value = res.data?.items || []
  } catch { ElMessage.error('加载知识点列表失败') }
  finally { loading.value = false }
}

function openDialog(row) {
  formRef.value?.resetFields()
  formChapterList.value = []
  if (row) {
    formMode.value = 'edit'
    editingId.value = row.id
    form.title = row.title
    form.examFrequency = row.examFrequency || ''
    form.coreContent = row.coreContent || ''
    form.formulas = row.formulas || ''
    form.typicalQuestions = row.typicalQuestions || ''
    form.commonMistakes = row.commonMistakes || ''
    form.reviewAdvice = row.reviewAdvice || ''
    form.sortOrder = row.sortOrder || 0
    // 异步回填 courseId / chapterId
    form.courseId = row.courseId
    form.chapterId = row.chapterId
    if (row.courseId) {
      loadChapters(row.courseId).then(items => { formChapterList.value = items })
    }
  } else {
    formMode.value = 'add'
    editingId.value = null
    Object.assign(form, {
      courseId: null, chapterId: null, title: '', examFrequency: '',
      coreContent: '', formulas: '', typicalQuestions: '', commonMistakes: '', reviewAdvice: '', sortOrder: 0
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
      courseId: form.courseId,
      chapterId: form.chapterId,
      title: form.title,
      examFrequency: form.examFrequency,
      coreContent: form.coreContent,
      formulas: form.formulas,
      typicalQuestions: form.typicalQuestions,
      commonMistakes: form.commonMistakes,
      reviewAdvice: form.reviewAdvice,
      sortOrder: form.sortOrder
    }
    let res
    if (formMode.value === 'add') {
      res = await adminCreateKnowledge(data)
    } else {
      res = await adminUpdateKnowledge(editingId.value, data)
    }
    if (res.code === 200) {
      ElMessage.success(formMode.value === 'add' ? '新增成功' : '编辑成功')
      dialogVisible.value = false
      fetchList()
    } else {
      ElMessage.error(res.message || '操作失败')
    }
  } catch { ElMessage.error('操作失败') }
  finally { submitting.value = false }
}

async function handleDelete(row) {
  try {
    await ElMessageBox.confirm(`确定删除知识点「${row.title}」吗？`, '删除确认', { type: 'warning' })
    const res = await adminDeleteKnowledge(row.id)
    if (res.code === 200) {
      ElMessage.success('删除成功')
      fetchList()
    } else {
      ElMessage.error(res.message || '删除失败')
    }
  } catch { /* 取消 */ }
}

function freqTagType(freq) {
  const map = { high: 'danger', '高频': 'danger', medium: 'warning', '中频': 'warning', low: 'info', '低频': 'info' }
  return map[freq] || 'info'
}
</script>
