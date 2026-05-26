import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query'
import { soundboardService } from '@/services/soundboard.service'
import type { PlaySoundboardPayload, SetSoundboardPayload } from '@/types'
import toast from 'react-hot-toast'

export function usePlaySoundboard() {
  return useMutation({
    mutationFn: (payload: PlaySoundboardPayload) => soundboardService.play(payload),
    onSuccess: () => {
      toast.success('Soundboard played')
    },
  })
}

export function useSoundboardDevices() {
  return useQuery({
    queryKey: ['soundboard-devices'],
    queryFn: () => soundboardService.get_devices(),
  })
}

export function useSetSoundboardDevice() {
  const queryClient = useQueryClient()
  
  return useMutation({
    mutationFn: (payload: SetSoundboardPayload) => soundboardService.set_device(payload),
    onSuccess: () => {
      toast.success('Audio output device updated')
      queryClient.invalidateQueries({ queryKey: ['soundboard-devices'] })
    },
    onError: () => {
      toast.error('Failed to update audio device')
    }
  })
}