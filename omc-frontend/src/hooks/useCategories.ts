import { useQuery, useMutation, useQueryClient } from '@tanstack/react-query'
import { categoryService } from '@/services/category.service'
import type { CreateCategoryPayload } from '@/types'
import toast from 'react-hot-toast'

export const CATEGORY_KEYS = {
  all: ['categories'] as const,
  detail: (id: number) => ['categories', id] as const,
}

export function useCategoryList() {
  return useQuery({
    queryKey: CATEGORY_KEYS.all,
    queryFn: categoryService.list,
  })
}

export function useCategoryDetail(id: number) {
  return useQuery({
    queryKey: CATEGORY_KEYS.detail(id),
    queryFn: () => categoryService.get(id),
    enabled: id > 0,
  })
}

export function useCreateCategory() {
  const qc = useQueryClient()
  return useMutation({
    mutationFn: (payload: CreateCategoryPayload) => categoryService.create(payload),
    onSuccess: () => {
      qc.invalidateQueries({ queryKey: CATEGORY_KEYS.all })
      toast.success('Category created')
    },
  })
}

export function useDeleteCategory() {
  const qc = useQueryClient()
  return useMutation({
    mutationFn: (id: number) => categoryService.delete(id),
    onSuccess: () => {
      qc.invalidateQueries({ queryKey: CATEGORY_KEYS.all })
      toast.success('Category deleted')
    },
  })
}

export function useAddMediaToCategory() {
  const qc = useQueryClient()
  return useMutation({
    mutationFn: ({ categoryId, mediaId }: { categoryId: number; mediaId: number }) =>
      categoryService.addMedia(categoryId, mediaId),
    onSuccess: (_data, { categoryId }) => {
      qc.invalidateQueries({ queryKey: CATEGORY_KEYS.detail(categoryId) })
      toast.success('Media added to category')
    },
  })
}

export function useRemoveMediaFromCategory() {
  const qc = useQueryClient()
  return useMutation({
    mutationFn: ({ categoryId, mediaId }: { categoryId: number; mediaId: number }) =>
      categoryService.removeMedia(categoryId, mediaId),
    onSuccess: (_data, { categoryId }) => {
      qc.invalidateQueries({ queryKey: CATEGORY_KEYS.detail(categoryId) })
      toast.success('Media removed from category')
    },
  })
}
