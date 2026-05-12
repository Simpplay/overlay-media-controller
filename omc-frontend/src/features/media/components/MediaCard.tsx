import { useState } from 'react'
import { Trash2, FileVideo, FileAudio, FileImage, File } from 'lucide-react'
import { useDeleteMedia, useMediaThumbnail } from '@/hooks/useMedia'
import { ConfirmDialog } from '@/shared/components/ConfirmDialog'
import { Badge } from '@/shared/components/Badge'
import type { Media } from '@/types'

function getMediaIcon(contentType?: string) {
  if (!contentType) return File
  if (contentType.startsWith('video/')) return FileVideo
  if (contentType.startsWith('audio/')) return FileAudio
  if (contentType.startsWith('image/')) return FileImage
  return File
}

interface MediaCardProps {
  media: Media
}

function MediaThumbnail({ media }: { media: Media }) {
  const { data: thumbnailUrl } = useMediaThumbnail(media.id)
  const [imgError, setImgError] = useState(false)
  const Icon = getMediaIcon(media.contentType)

  if (thumbnailUrl && !imgError) {
    return (
      <img
        src={thumbnailUrl}
        alt={media.title}
        className="w-full h-full object-cover"
        onError={() => setImgError(true)}
      />
    )
  }

  return (
    <div className="flex flex-col items-center gap-2">
      <Icon className="w-10 h-10 text-zinc-600" />
      <span className="text-xs text-zinc-600 capitalize">
        {media.contentType?.split('/')[0] ?? 'file'}
      </span>
    </div>
  )
}

export function MediaCard({ media }: MediaCardProps) {
  if (!media) return null
  console.log('Rendering MediaCard for media:', media) // Debug log
  console.log('Media content type:', media.contentType) // Debug log
  const [confirmOpen, setConfirmOpen] = useState(false)
  const deleteMutation = useDeleteMedia()

  return (
    <>
      <div className="glass rounded-xl overflow-hidden card-hover group relative">
        <div className="aspect-video bg-zinc-900 flex items-center justify-center relative overflow-hidden">
          <MediaThumbnail media={media} />

          <div className="absolute inset-0 bg-black/60 opacity-0 group-hover:opacity-100 transition-opacity flex items-center justify-center">
            <button
              onClick={() => setConfirmOpen(true)}
              className="p-2 rounded-lg bg-red-600/80 hover:bg-red-600 text-white transition-colors"
            >
              <Trash2 className="w-4 h-4" />
            </button>
          </div>
        </div>

        <div className="px-3 py-2.5 space-y-1">
          <p className="text-sm font-medium text-zinc-100 truncate" title={media.title}>
            {media.title || media.filename}
          </p>
          <div className="flex items-center justify-between gap-2">
            <Badge variant="default">
              {media.contentType?.split('/')[1] ?? media.contentType ?? 'file'}
            </Badge>
            {media.categories && media.categories.length > 0 && (
              <span className="text-xs text-zinc-500 truncate">
                {media.categories[0]}
              </span>
            )}
          </div>
        </div>
      </div>

      <ConfirmDialog
        open={confirmOpen}
        onClose={() => setConfirmOpen(false)}
        onConfirm={() =>
          deleteMutation.mutate(media.id, { onSuccess: () => setConfirmOpen(false) })
        }
        title="Delete Media"
        message={`Are you sure you want to delete "${media.title || media.filename}"? This action cannot be undone.`}
        loading={deleteMutation.isPending}
      />
    </>
  )
}
