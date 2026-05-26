import api from './api'
import type {
  PlaySoundboardPayload,
  PlaySoundboardResponse,
} from '@/types'

export const soundboardService = {
  play: async (payload: PlaySoundboardPayload): Promise<PlaySoundboardResponse> => {
    const res = await api.post<PlaySoundboardResponse>('/soundboard', payload)
    return res.data
  },
}
