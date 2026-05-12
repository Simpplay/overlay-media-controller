import { useQuery, useMutation, useQueryClient } from '@tanstack/react-query'
import { overlayService } from '@/services/overlay.service'
import type { CreateOverlayPayload, UpdateOverlayPayload } from '@/types'
import toast from 'react-hot-toast'

export const OVERLAY_KEYS = {
  all: ['overlays'] as const,
}

export function useOverlayList() {
  return useQuery({
    queryKey: OVERLAY_KEYS.all,
    queryFn: overlayService.list,
    refetchInterval: 3000,
  })
}

export function useCreateOverlay() {
  const qc = useQueryClient()
  return useMutation({
    mutationFn: (payload: CreateOverlayPayload) => overlayService.create(payload),
    onSuccess: () => {
      qc.invalidateQueries({ queryKey: OVERLAY_KEYS.all })
      toast.success('Overlay created')
    },
  })
}

export function useUpdateOverlay() {
  const qc = useQueryClient()
  return useMutation({
    mutationFn: ({ id, payload }: { id: number; payload: UpdateOverlayPayload }) =>
      overlayService.update(id, payload),
    onSuccess: () => {
      qc.invalidateQueries({ queryKey: OVERLAY_KEYS.all })
    },
  })
}

export function useDeleteOverlay() {
  const qc = useQueryClient()
  return useMutation({
    mutationFn: (id: number) => overlayService.delete(id),
    onSuccess: () => {
      qc.invalidateQueries({ queryKey: OVERLAY_KEYS.all })
      toast.success('Overlay deleted')
    },
  })
}
