import api from './api'
import type {
  PlaySoundboardPayload,
  PlaySoundboardResponse,
  SoundboardDevice,
  SetSoundboardPayload
} from '@/types'

export const soundboardService = {
  play: async (payload: PlaySoundboardPayload): Promise<PlaySoundboardResponse> => {
    const res = await api.post<PlaySoundboardResponse>('/soundboard', payload)
    return res.data
  },

  get_devices: async (): Promise<SoundboardDevice[]> => {
    const res = await api.get<SoundboardDevice[]>('/soundboard/devices')
    return res.data
  },

  set_device: async (payload: SetSoundboardPayload): Promise<void> => {
    await api.put('/soundboard/devices', payload)
  }
}
