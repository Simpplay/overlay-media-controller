import { useState, useMemo } from 'react'
import {
  Upload, Search, LayoutGrid, List, FileX,
  Trash2, FileVideo, FileAudio, FileImage, File,
} from 'lucide-react'
import { useMediaList, useDeleteMedia } from '@/hooks/useMedia'
import { MediaCard } from './components/MediaCard'
import { MediaUploadModal } from './components/MediaUploadModal'
import { Button } from '@/shared/components/Button'
import { Input } from '@/shared/components/Input'
import { SkeletonGrid } from '@/shared/components/SkeletonCard'
import { EmptyState } from '@/shared/components/EmptyState'
import { ConfirmDialog } from '@/shared/components/ConfirmDialog'
import type { Media } from '@/types'

const CONTENT_TYPES = ['all', 'video', 'audio', 'image'] as const
type TypeFilter = (typeof CONTENT_TYPES)[number]

function getMediaIcon(contentType: string) {
  if (contentType.startsWith('video/')) return FileVideo
  if (contentType.startsWith('audio/')) return FileAudio
  if (contentType.startsWith('image/')) return FileImage
  return File
}

function filterByType(media: Media[], type: TypeFilter): Media[] {
  if (type === 'all') return media
  return media.filter((m) => (m.contentType)?.startsWith(`${type}/`))
}

function MediaListRow({ media }: { media: Media }) {
  const [confirmOpen, setConfirmOpen] = useState(false)
  const { mutate: del, isPending } = useDeleteMedia()
  const Icon = getMediaIcon( media.contentType || '')

  return (
    <>
      <div className="glass rounded-xl px-4 py-3 flex items-center gap-4 card-hover group">
        <div className="w-10 h-10 bg-zinc-900 rounded-lg flex items-center justify-center flex-shrink-0">
          <Icon className="w-5 h-5 text-zinc-500" />
        </div>
        <div className="flex-1 min-w-0">
          <p className="text-sm font-medium text-zinc-100 truncate">{media.title || media.filename}</p>
          <p className="text-xs text-zinc-500 mt-0.5">{media.contentType || 'Unknown'}</p>
        </div>
        {media.categories.length > 0 && (
          <span className="text-xs text-zinc-500 hidden sm:block truncate max-w-24">
            {media.categories.join(', ')}
          </span>
        )}
        <Button
          variant="danger"
          size="sm"
          className="opacity-0 group-hover:opacity-100 transition-opacity"
          onClick={() => setConfirmOpen(true)}
        >
          <Trash2 className="w-3.5 h-3.5" />
        </Button>
      </div>
      <ConfirmDialog
        open={confirmOpen}
        onClose={() => setConfirmOpen(false)}
        onConfirm={() => del(media.id, { onSuccess: () => setConfirmOpen(false) })}
        title="Delete Media"
        message={`Delete "${media.title || media.filename}"? This cannot be undone.`}
        loading={isPending}
      />
    </>
  )
}

export function MediaPage() {
  const [search, setSearch] = useState('')
  const [typeFilter, setTypeFilter] = useState<TypeFilter>('all')
  const [uploadOpen, setUploadOpen] = useState(false)
  const [viewMode, setViewMode] = useState<'grid' | 'list'>('grid')

  // Pass search query to the server; type filtering stays client-side
  const { data: media = [], isLoading } = useMediaList(
    search.trim() ? { query: search.trim() } : undefined
  )

  const filtered = useMemo(() => filterByType(media, typeFilter), [media, typeFilter])

  return (
    <div className="space-y-5">
      <div className="flex flex-col sm:flex-row sm:items-center gap-4">
        <div>
          <h1 className="text-2xl font-bold text-zinc-100">Media Library</h1>
          <p className="text-sm text-zinc-500 mt-0.5">{media.length} files total</p>
        </div>
        <div className="sm:ml-auto">
          <Button
            variant="primary"
            icon={<Upload className="w-4 h-4" />}
            onClick={() => setUploadOpen(true)}
          >
            Upload
          </Button>
        </div>
      </div>

      <div className="flex flex-col sm:flex-row gap-3">
        <div className="relative flex-1">
          <Search className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-zinc-500" />
          <Input
            className="pl-9"
            placeholder="Search media..."
            value={search}
            onChange={(e) => setSearch(e.target.value)}
          />
        </div>
        <div className="flex gap-1 glass rounded-lg p-1">
          {CONTENT_TYPES.map((t) => (
            <button
              key={t}
              onClick={() => setTypeFilter(t)}
              className={[
                'px-3 py-1.5 rounded-md text-sm font-medium capitalize transition-colors duration-150',
                typeFilter === t
                  ? 'bg-violet-600 text-white'
                  : 'text-zinc-400 hover:text-zinc-100 hover:bg-zinc-800',
              ].join(' ')}
            >
              {t}
            </button>
          ))}
        </div>
        <div className="flex gap-1 glass rounded-lg p-1">
          <button
            onClick={() => setViewMode('grid')}
            className={[
              'p-1.5 rounded-md transition-colors',
              viewMode === 'grid' ? 'bg-zinc-700 text-zinc-100' : 'text-zinc-500 hover:text-zinc-300',
            ].join(' ')}
          >
            <LayoutGrid className="w-4 h-4" />
          </button>
          <button
            onClick={() => setViewMode('list')}
            className={[
              'p-1.5 rounded-md transition-colors',
              viewMode === 'list' ? 'bg-zinc-700 text-zinc-100' : 'text-zinc-500 hover:text-zinc-300',
            ].join(' ')}
          >
            <List className="w-4 h-4" />
          </button>
        </div>
      </div>

      {isLoading ? (
        <SkeletonGrid count={12} />
      ) : filtered.length === 0 ? (
        <EmptyState
          icon={<FileX className="w-12 h-12" />}
          title={search || typeFilter !== 'all' ? 'No results found' : 'No media yet'}
          description={
            search || typeFilter !== 'all'
              ? 'Try adjusting your search or filter.'
              : 'Upload your first media file to get started.'
          }
          action={
            !search && typeFilter === 'all' ? (
              <Button
                variant="primary"
                icon={<Upload className="w-4 h-4" />}
                onClick={() => setUploadOpen(true)}
              >
                Upload Media
              </Button>
            ) : undefined
          }
        />
      ) : viewMode === 'grid' ? (
        <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 lg:grid-cols-5 xl:grid-cols-6 gap-3">
          {filtered.map((m) => (
            <MediaCard key={m.id} media={m} />
          ))}
        </div>
      ) : (
        <div className="space-y-2">
          {filtered.map((m) => (
            <MediaListRow key={m.id} media={m} />
          ))}
        </div>
      )}

      <MediaUploadModal open={uploadOpen} onClose={() => setUploadOpen(false)} />
    </div>
  )
}
