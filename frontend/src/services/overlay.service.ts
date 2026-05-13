import api from './api'
import type {
  Overlay,
  CreateOverlayPayload,
  CreateOverlayResponse,
  UpdateOverlayPayload,
  UpdateOverlayResponse,
} from '@/types'

export const overlayService = {
  list: async (): Promise<Overlay[]> => {
    const res = await api.get<Overlay[]>('/overlays')
    return res.data
  },

  create: async (payload: CreateOverlayPayload): Promise<CreateOverlayResponse> => {
    const res = await api.post<CreateOverlayResponse>('/overlays', payload)
    return res.data
  },

  update: async (id: number, payload: UpdateOverlayPayload): Promise<UpdateOverlayResponse> => {
    const res = await api.patch<UpdateOverlayResponse>(`/overlays/${id}`, payload)
    return res.data
  },

  delete: async (id: number): Promise<void> => {
    await api.delete(`/overlays/${id}`)
  },
}
