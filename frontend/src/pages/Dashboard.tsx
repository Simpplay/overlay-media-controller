import { Link } from 'react-router-dom'
import { LayoutGrid, FolderOpen, Layers, Eye, FileVideo, ArrowRight } from 'lucide-react'
import { useMediaList } from '@/hooks/useMedia'
import { useCategoryList } from '@/hooks/useCategories'
import { useOverlayList } from '@/hooks/useOverlays'
import { Badge } from '@/shared/components/Badge'

interface StatCardProps {
  icon: React.ReactNode
  label: string
  value: number | string
  to: string
  badge?: React.ReactNode
}

function StatCard({ icon, label, value, to, badge }: StatCardProps) {
  return (
    <Link to={to} className="glass rounded-xl p-5 card-hover flex flex-col gap-3 group">
      <div className="flex items-center justify-between">
        <div className="w-10 h-10 rounded-xl bg-violet-600/15 flex items-center justify-center text-violet-400">
          {icon}
        </div>
        {badge}
      </div>
      <div>
        <p className="text-3xl font-bold text-zinc-100">{value}</p>
        <p className="text-sm text-zinc-500 mt-0.5">{label}</p>
      </div>
      <div className="flex items-center gap-1 text-xs text-zinc-600 group-hover:text-violet-400 transition-colors mt-auto">
        View all
        <ArrowRight className="w-3.5 h-3.5" />
      </div>
    </Link>
  )
}

export function Dashboard() {
  const { data: media = [], isLoading: loadingMedia } = useMediaList()
  const { data: categories = [], isLoading: loadingCats } = useCategoryList()
  const { data: overlays = [], isLoading: loadingOverlays } = useOverlayList()

  const activeOverlays = overlays.filter((o) => o.state === 'active').length

  return (
    <div className="space-y-6">
      <div className="glass rounded-2xl p-6 sm:p-8 flex flex-col sm:flex-row gap-6 items-start">
        <div className="flex-1">
          <h1 className="text-3xl font-bold text-zinc-100 tracking-tight">
            Overlay Media Controller
          </h1>
          <p className="text-zinc-500 mt-2 text-sm leading-relaxed max-w-md">
            Manage your media library, organize content into categories, and control overlay windows — all from a single dashboard.
          </p>
        </div>
        <div className="flex gap-2 flex-wrap">
          <Badge variant={activeOverlays > 0 ? 'green' : 'default'}>
            <Eye className="w-3 h-3" />
            {activeOverlays} active
          </Badge>
          <Badge variant="violet">
            <Layers className="w-3 h-3" />
            API: localhost:8080
          </Badge>
        </div>
      </div>

      <div className="grid grid-cols-1 sm:grid-cols-3 gap-3">
        <StatCard
          icon={<LayoutGrid className="w-5 h-5" />}
          label="Media files"
          value={loadingMedia ? '—' : media.length}
          to="/media"
        />
        <StatCard
          icon={<FolderOpen className="w-5 h-5" />}
          label="Categories"
          value={loadingCats ? '—' : categories.length}
          to="/categories"
        />
        <StatCard
          icon={<Layers className="w-5 h-5" />}
          label="Overlays"
          value={loadingOverlays ? '—' : overlays.length}
          to="/overlays"
          badge={
            activeOverlays > 0 ? (
              <Badge variant="green">
                <Eye className="w-3 h-3" />
                {activeOverlays} active
              </Badge>
            ) : undefined
          }
        />
      </div>

      {media.length > 0 && (
        <div className="space-y-3">
          <div className="flex items-center justify-between">
            <h2 className="text-base font-semibold text-zinc-200">Recent Media</h2>
            <Link to="/media" className="text-sm text-violet-400 hover:text-violet-300 flex items-center gap-1">
              View all <ArrowRight className="w-4 h-4" />
            </Link>
          </div>
          <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-5 gap-2">
            {media.slice(0, 5).map((m) => (
              <div key={m.id} className="glass rounded-lg p-3 flex flex-col items-center gap-2 text-center">
                <FileVideo className="w-6 h-6 text-zinc-600" />
                <p className="text-xs text-zinc-400 truncate w-full">{m.title || m.filename}</p>
              </div>
            ))}
          </div>
        </div>
      )}

      {overlays.length > 0 && (
        <div className="space-y-3">
          <div className="flex items-center justify-between">
            <h2 className="text-base font-semibold text-zinc-200">Overlays</h2>
            <Link to="/overlays" className="text-sm text-violet-400 hover:text-violet-300 flex items-center gap-1">
              Manage <ArrowRight className="w-4 h-4" />
            </Link>
          </div>
          <div className="space-y-2">
            {overlays.slice(0, 4).map((o) => (
              <div key={o.id} className="glass rounded-lg px-4 py-3 flex items-center gap-3">
                <div
                  className={[
                    'w-2 h-2 rounded-full flex-shrink-0',
                    o.state === 'active' ? 'bg-emerald-400' : 'bg-zinc-600',
                  ].join(' ')}
                />
                <span className="text-sm text-zinc-300 flex-1">Overlay #{o.id}</span>
                <span className="text-xs text-zinc-600 font-mono hidden sm:block">
                  {o.size.width}×{o.size.height}
                </span>
                <Badge variant={o.state === 'active' ? 'green' : 'default'}>
                  {o.state}
                </Badge>
              </div>
            ))}
          </div>
        </div>
      )}
    </div>
  )
}
