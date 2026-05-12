import api from './api'
import type {
  CategorySummary,
  CategoryDetail,
  CreateCategoryPayload,
} from '@/types'

export const categoryService = {
  list: async (): Promise<CategorySummary[]> => {
    const res = await api.get<CategorySummary[]>('/categories')
    return res.data
  },

  get: async (id: number): Promise<CategoryDetail> => {
    const res = await api.get<CategoryDetail>(`/categories/${id}`)
    return res.data
  },

  create: async (payload: CreateCategoryPayload): Promise<CategorySummary> => {
    const res = await api.post<CategorySummary>('/categories', payload)
    return res.data
  },

  delete: async (id: number): Promise<void> => {
    await api.delete(`/categories/${id}`)
  },

  addMedia: async (categoryId: number, mediaId: number): Promise<void> => {
    await api.put(`/categories/${categoryId}/media/${mediaId}`)
  },

  removeMedia: async (categoryId: number, mediaId: number): Promise<void> => {
    await api.delete(`/categories/${categoryId}/media/${mediaId}`)
  },
}
