import { useState } from 'react'
import { Layers } from 'lucide-react'
import { Modal } from '@/shared/components/Modal'
import { Button } from '@/shared/components/Button'
import { Input } from '@/shared/components/Input'
import { useCreateOverlay } from '@/hooks/useOverlays'
import { useMediaList } from '@/hooks/useMedia'
import type { CreateOverlayPayload } from '@/types'

interface CreateOverlayModalProps {
  open: boolean
  onClose: () => void
}

export function CreateOverlayModal({ open, onClose }: CreateOverlayModalProps) {
  const [mediaId, setMediaId] = useState<number | ''>('')
  const [fullscreen, setFullscreen] = useState(false)
  const [posX, setPosX] = useState('100')
  const [posY, setPosY] = useState('100')
  const [width, setWidth] = useState('800')
  const [height, setHeight] = useState('600')
  const [error, setError] = useState('')

  const createMutation = useCreateOverlay()
  const { data: media = [] } = useMediaList()

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault()
    if (mediaId === '') { setError('Select a media item'); return }

    const payload: CreateOverlayPayload = {
      media_id: mediaId,
      fullscreen,
      position: { x: Number(posX), y: Number(posY) },
      size: { width: Number(width), height: Number(height) },
    }

    createMutation.mutate(payload, {
      onSuccess: () => {
        resetForm()
        onClose()
      },
    })
  }

  const resetForm = () => {
    setMediaId('')
    setFullscreen(false)
    setPosX('100')
    setPosY('100')
    setWidth('800')
    setHeight('600')
    setError('')
  }

  const handleClose = () => {
    resetForm()
    onClose()
  }

  return (
    <Modal open={open} onClose={handleClose} title="Create Overlay" size="md">
      <form onSubmit={handleSubmit} className="space-y-4">
        {/* Media picker */}
        <div className="flex flex-col gap-1.5">
          <label className="text-sm font-medium text-zinc-300">Media item</label>
          <select
            value={mediaId}
            onChange={(e) => { setMediaId(e.target.value === '' ? '' : Number(e.target.value)); setError('') }}
            className="w-full px-3 py-2 rounded-lg bg-zinc-900 border border-zinc-700 text-sm text-zinc-100 focus:outline-none focus:ring-2 focus:ring-violet-500 focus:border-transparent"
          >
            <option value="">Select a media item…</option>
            {media.map((m) => (
              <option key={m.id} value={m.id}>
                {m.title || m.filename}
              </option>
            ))}
          </select>
          {error && <p className="text-xs text-red-400">{error}</p>}
        </div>

        {/* Position */}
        <div className="grid grid-cols-2 gap-3">
          <Input label="X position" type="number" value={posX} onChange={(e) => setPosX(e.target.value)} />
          <Input label="Y position" type="number" value={posY} onChange={(e) => setPosY(e.target.value)} />
        </div>

        {/* Size */}
        <div className="grid grid-cols-2 gap-3">
          <Input label="Width" type="number" value={width} onChange={(e) => setWidth(e.target.value)} />
          <Input label="Height" type="number" value={height} onChange={(e) => setHeight(e.target.value)} />
        </div>

        {/* Fullscreen toggle */}
        <label className="flex items-center gap-3 cursor-pointer">
          <div
            onClick={() => setFullscreen((v) => !v)}
            className={[
              'w-9 h-5 rounded-full transition-colors',
              fullscreen ? 'bg-violet-600' : 'bg-zinc-700',
            ].join(' ')}
          >
            <div
              className={[
                'w-3.5 h-3.5 bg-white rounded-full mt-[3px] transition-transform mx-[3px]',
                fullscreen ? 'translate-x-4' : 'translate-x-0',
              ].join(' ')}
            />
          </div>
          <span className="text-sm text-zinc-300">Fullscreen</span>
        </label>

        <div className="flex gap-2 justify-end pt-2">
          <Button variant="ghost" type="button" onClick={handleClose}>Cancel</Button>
          <Button
            variant="primary"
            type="submit"
            loading={createMutation.isPending}
            icon={<Layers className="w-4 h-4" />}
          >
            Create Overlay
          </Button>
        </div>
      </form>
    </Modal>
  )
}
