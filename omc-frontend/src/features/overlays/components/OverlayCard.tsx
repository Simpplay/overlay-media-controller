import { useState } from 'react'
import { Maximize2, Minimize2, Trash2, GripVertical } from 'lucide-react'
import { Badge } from '@/shared/components/Badge'
import { Button } from '@/shared/components/Button'
import { ConfirmDialog } from '@/shared/components/ConfirmDialog'
import { useUpdateOverlay, useDeleteOverlay } from '@/hooks/useOverlays'
import type { Overlay } from '@/types'

interface OverlayCardProps {
  overlay: Overlay
}

export function OverlayCard({ overlay }: OverlayCardProps) {
  const [confirmOpen, setConfirmOpen] = useState(false)
  const updateMutation = useUpdateOverlay()
  const deleteMutation = useDeleteOverlay()

  const toggleFullscreen = () => {
    updateMutation.mutate({
      id: overlay.id,
      payload: { fullscreen: !overlay.fullscreen },
    })
  }

  const isActive = overlay.state === 'active'

  return (
    <>
      <div className="glass rounded-xl p-4 card-hover space-y-3">
        <div className="flex items-start gap-3">
          <GripVertical className="w-4 h-4 text-zinc-700 mt-0.5 flex-shrink-0" />
          <div className="flex-1 min-w-0">
            <p className="font-semibold text-zinc-100 text-sm">Overlay #{overlay.id}</p>
            <p className="text-xs text-zinc-500 mt-0.5">
              Media ID: {overlay.media_id}
            </p>
          </div>
          <button
            onClick={() => setConfirmOpen(true)}
            className="p-1.5 text-zinc-600 hover:text-red-400 hover:bg-red-600/10 rounded-lg transition-colors flex-shrink-0"
          >
            <Trash2 className="w-3.5 h-3.5" />
          </button>
        </div>

        <div className="grid grid-cols-2 gap-2 text-xs">
          <div className="bg-zinc-900 rounded-lg px-2.5 py-1.5">
            <span className="text-zinc-600">Position</span>
            <p className="text-zinc-300 font-mono mt-0.5">
              {overlay.position.x}, {overlay.position.y}
            </p>
          </div>
          <div className="bg-zinc-900 rounded-lg px-2.5 py-1.5">
            <span className="text-zinc-600">Size</span>
            <p className="text-zinc-300 font-mono mt-0.5">
              {overlay.size.width} × {overlay.size.height}
            </p>
          </div>
        </div>

        <div className="flex items-center gap-2 flex-wrap">
          <Badge variant={isActive ? 'green' : 'default'}>
            {overlay.state}
          </Badge>
          {overlay.fullscreen && <Badge variant="violet">Fullscreen</Badge>}
          <div className="ml-auto">
            <Button
              size="sm"
              variant="ghost"
              onClick={toggleFullscreen}
              loading={updateMutation.isPending}
              title={overlay.fullscreen ? 'Exit fullscreen' : 'Fullscreen'}
            >
              {overlay.fullscreen
                ? <Minimize2 className="w-3.5 h-3.5" />
                : <Maximize2 className="w-3.5 h-3.5" />}
            </Button>
          </div>
        </div>
      </div>

      <ConfirmDialog
        open={confirmOpen}
        onClose={() => setConfirmOpen(false)}
        onConfirm={() =>
          deleteMutation.mutate(overlay.id, { onSuccess: () => setConfirmOpen(false) })
        }
        title="Delete Overlay"
        message={`Delete overlay #${overlay.id}? This action cannot be undone.`}
        loading={deleteMutation.isPending}
      />
    </>
  )
}
