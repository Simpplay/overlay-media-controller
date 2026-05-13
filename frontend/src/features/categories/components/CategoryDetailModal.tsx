import { useState } from 'react'
import { Plus, X, FileVideo, FileAudio, FileImage, File } from 'lucide-react'
import { Modal } from '@/shared/components/Modal'
import { Button } from '@/shared/components/Button'
import { Badge } from '@/shared/components/Badge'
import { EmptyState } from '@/shared/components/EmptyState'
import { useCategoryDetail, useAddMediaToCategory, useRemoveMediaFromCategory } from '@/hooks/useCategories'
import { useMediaList } from '@/hooks/useMedia'
import type { CategorySummary } from '@/types'

function getMediaIcon(contentType: string) {
  if (!contentType) return File
  if (contentType.startsWith('video/')) return FileVideo
  if (contentType.startsWith('audio/')) return FileAudio
  if (contentType.startsWith('image/')) return FileImage
  return File
}

interface CategoryDetailModalProps {
  category: CategorySummary | null
  onClose: () => void
}

export function CategoryDetailModal({ category, onClose }: CategoryDetailModalProps) {
  const [showAddPanel, setShowAddPanel] = useState(false)
  const { data: detail } = useCategoryDetail(category?.id ?? 0)
  const { data: allMedia = [] } = useMediaList()
  const addMedia = useAddMediaToCategory()
  const removeMedia = useRemoveMediaFromCategory()

  const assignedIds = new Set((detail?.media ?? []).map((m) => m.id))
  const available = allMedia.filter((m) => !assignedIds.has(m.id))

  if (!category) return null

  return (
    <Modal open={Boolean(category)} onClose={onClose} title={category.name} size="lg">
      <div className="space-y-4">
        <div className="flex items-center justify-between">
          <p className="text-sm text-zinc-500">
            {(detail?.media ?? []).length} media items
          </p>
          <Button
            variant="secondary"
            size="sm"
            icon={<Plus className="w-3.5 h-3.5" />}
            onClick={() => setShowAddPanel(!showAddPanel)}
          >
            Add Media
          </Button>
        </div>

        {showAddPanel && (
          <div className="bg-zinc-900 rounded-xl p-3 space-y-1 max-h-40 overflow-y-auto border border-zinc-800">
            {available.length === 0 ? (
              <p className="text-sm text-zinc-500 text-center py-3">All media already assigned</p>
            ) : (
              available.map((m) => {
                const Icon = getMediaIcon(m.contentType)
                return (
                  <button
                    key={m.id}
                    onClick={() => addMedia.mutate({ categoryId: category.id, mediaId: m.id })}
                    className="w-full flex items-center gap-3 px-3 py-2 rounded-lg hover:bg-zinc-800 transition-colors text-left"
                  >
                    <Icon className="w-4 h-4 text-zinc-500 flex-shrink-0" />
                    <span className="text-sm text-zinc-300 truncate flex-1">
                      {m.title || m.filename}
                    </span>
                    <Plus className="w-3.5 h-3.5 text-zinc-500 flex-shrink-0" />
                  </button>
                )
              })
            )}
          </div>
        )}

        {(detail?.media ?? []).length === 0 ? (
          <EmptyState
            icon={<File className="w-10 h-10" />}
            title="No media in this category"
            description="Add media files using the button above."
          />
        ) : (
          <ul className="space-y-2 max-h-72 overflow-y-auto">
            {(detail?.media ?? []).map((m) => {
              const Icon = getMediaIcon(m.contentType)
              return (
                <li key={m.id} className="flex items-center gap-3 px-3 py-2.5 glass rounded-lg group">
                  <Icon className="w-4 h-4 text-zinc-500 flex-shrink-0" />
                  <span className="flex-1 text-sm text-zinc-200 truncate">
                    {m.title || m.filename}
                  </span>
                  <Badge variant="default">
                    {m.contentType.split('/')[1]}
                  </Badge>
                  <button
                    onClick={() => removeMedia.mutate({ categoryId: category.id, mediaId: m.id })}
                    className="text-zinc-600 hover:text-red-400 transition-colors opacity-0 group-hover:opacity-100"
                  >
                    <X className="w-4 h-4" />
                  </button>
                </li>
              )
            })}
          </ul>
        )}

        <div className="flex justify-end pt-2">
          <Button variant="ghost" onClick={onClose}>Close</Button>
        </div>
      </div>
    </Modal>
  )
}
