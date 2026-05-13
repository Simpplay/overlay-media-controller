import { useQuery, useMutation, useQueryClient } from '@tanstack/react-query'
import { mediaService } from '@/services/media.service'
import type { MediaListParams, UploadMediaParams } from '@/types'
import toast from 'react-hot-toast'

export const MEDIA_KEYS = {
  all: (params?: MediaListParams) => params ? ['media', params] as const : ['media'] as const,
  detail: (id: number) => ['media', id] as const,
  thumbnail: (id: number) => ['media', id, 'thumbnail'] as const,
}

export function useMediaList(params?: MediaListParams) {
  return useQuery({
    queryKey: MEDIA_KEYS.all(params),
    queryFn: () => mediaService.list(params),
  })
}

export function useMediaDetail(id: number) {
  return useQuery({
    queryKey: MEDIA_KEYS.detail(id),
    queryFn: () => mediaService.get(id),
    enabled: id > 0,
  })
}

export function useMediaThumbnail(id: number) {
  return useQuery({
    queryKey: MEDIA_KEYS.thumbnail(id),
    queryFn: () => mediaService.getThumbnail(id),
    enabled: id > 0,
    staleTime: 60_000,
  })
}

export function useUploadMedia() {
  const qc = useQueryClient()
  return useMutation({
    mutationFn: (params: UploadMediaParams) => mediaService.upload(params),
    onSuccess: () => {
      qc.invalidateQueries({ queryKey: ['media'] })
      toast.success('Media uploaded successfully')
    },
  })
}

export function useDeleteMedia() {
  const qc = useQueryClient()
  return useMutation({
    mutationFn: (id: number) => mediaService.delete(id),
    onSuccess: () => {
      qc.invalidateQueries({ queryKey: ['media'] })
      toast.success('Media deleted')
    },
  })
}

export function useRenameMedia() {
  const qc = useQueryClient()
  return useMutation({
    mutationFn: ({ id, title }: { id: number; title: string }) => mediaService.patch(id, { title: title }),
    onSuccess: () => {
      qc.invalidateQueries({ queryKey: ['media'] })
      toast.success('Media renamed')
    },
  })
}
