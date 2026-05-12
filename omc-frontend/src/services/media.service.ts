import api from './api'
import type {
  Media,
  MediaListParams,
  UploadMediaParams,
  UploadMediaResponse
} from '@/types'

export const mediaService = {
  list: async (params?: MediaListParams): Promise<Media[]> => {
    const res = await api.get<any[]>('/media', { params })
    // Normalize server response to frontend `Media` shape
    return res.data.map((item) => ({
      id: item.id,
      title: item.title ?? item.name ?? '',
      filename: item.filename,
      // backend may return `contentType` (camelCase) or `content_type` (snake_case)
      contentType: item.contentType ?? '',
      // backend returns `categoryIds`; keep fallback for legacy `categories`
      categories: Array.isArray(item.categoryIds)
        ? item.categoryIds
        : (Array.isArray(item.categories) ? item.categories : []),
    }))
  },

  get: async (id: number): Promise<Media> => {
    const res = await api.get<any>(`/media/${id}`)
    const item = res.data
    return {
      id: item.id,
      title: item.title ?? item.name ?? '',
      filename: item.filename,
      contentType: item.contentType ?? '',
      categories: Array.isArray(item.categoryIds)
        ? item.categoryIds
        : (Array.isArray(item.categories) ? item.categories : []),
    }
  },

  upload: async ({ file, title, description }: UploadMediaParams): Promise<UploadMediaResponse> => {
    const form = new FormData()
    form.append('media', file)
    if (title) form.append('title', title)
    if (description) form.append('description', description)
    const res = await api.post<UploadMediaResponse>('/media', form)
    return res.data
  },

  delete: async (id: number): Promise<void> => {
    await api.delete(`/media/${id}`)
  },

  getThumbnail: async (id: number): Promise<string> => {
    const res = await api.get(`/media/${id}/thumbnail`, { responseType: 'blob' })
    return URL.createObjectURL(res.data)
  }
}
