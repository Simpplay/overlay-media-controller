import { useState } from 'react'
import { Plus, Layers, Eye, RefreshCw } from 'lucide-react'
import { useOverlayList } from '@/hooks/useOverlays'
import { OverlayCard } from './components/OverlayCard'
import { CreateOverlayModal } from './components/CreateOverlayModal'
import { Button } from '@/shared/components/Button'
import { Badge } from '@/shared/components/Badge'
import { EmptyState } from '@/shared/components/EmptyState'
import { SkeletonRow } from '@/shared/components/SkeletonCard'

export function OverlaysPage() {
  const { data: overlays = [], isLoading, refetch, isFetching } = useOverlayList()
  const [createOpen, setCreateOpen] = useState(false)

  const active = overlays.filter((o) => o.state === 'active').length
  const total = overlays.length

  return (
    <div className="space-y-5">
      <div className="flex items-center gap-4">
        <div>
          <h1 className="text-2xl font-bold text-zinc-100">Overlays</h1>
          <div className="flex items-center gap-2 mt-1">
            <Badge variant="green">
              <Eye className="w-3 h-3" />
              {active} active
            </Badge>
            <Badge variant="default">{total} total</Badge>
          </div>
        </div>
        <div className="ml-auto flex gap-2">
          <Button
            variant="ghost"
            size="sm"
            icon={<RefreshCw className={['w-4 h-4', isFetching ? 'animate-spin' : ''].join(' ')} />}
            onClick={() => refetch()}
          >
            Sync
          </Button>
          <Button
            variant="primary"
            icon={<Plus className="w-4 h-4" />}
            onClick={() => setCreateOpen(true)}
          >
            New Overlay
          </Button>
        </div>
      </div>

      {total > 0 && (
        <div className="glass rounded-xl px-4 py-3 flex items-center gap-3 text-sm text-zinc-400">
          <Eye className="w-4 h-4 text-emerald-400" />
          <span>{active} of {total} overlays are active</span>
        </div>
      )}

      {isLoading ? (
        <div className="grid sm:grid-cols-2 lg:grid-cols-3 gap-3">
          {Array.from({ length: 3 }).map((_, i) => <SkeletonRow key={i} />)}
        </div>
      ) : overlays.length === 0 ? (
        <EmptyState
          icon={<Layers className="w-12 h-12" />}
          title="No overlays yet"
          description="Create overlay windows linked to media items."
          action={
            <Button variant="primary" icon={<Plus className="w-4 h-4" />} onClick={() => setCreateOpen(true)}>
              Create Overlay
            </Button>
          }
        />
      ) : (
        <div className="grid sm:grid-cols-2 lg:grid-cols-3 gap-3">
          {overlays.map((overlay) => (
            <OverlayCard key={overlay.id} overlay={overlay} />
          ))}
        </div>
      )}

      <CreateOverlayModal open={createOpen} onClose={() => setCreateOpen(false)} />
    </div>
  )
}
