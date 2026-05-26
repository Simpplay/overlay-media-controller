import { useMutation } from '@tanstack/react-query'
import { soundboardService } from '@/services/soundboard.service'
import type { PlaySoundboardPayload } from '@/types'
import toast from 'react-hot-toast'

export function usePlaySoundboard() {
  return useMutation({
    mutationFn: (payload: PlaySoundboardPayload) => soundboardService.play(payload),
    onSuccess: () => {
      toast.success('Soundboard played')
    },
  })
}
