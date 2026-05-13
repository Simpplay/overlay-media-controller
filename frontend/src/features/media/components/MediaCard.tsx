import { useState, useRef, useEffect } from 'react'
import { Trash2, FileVideo, FileAudio, FileImage, File, Pencil, TvMinimalPlay } from 'lucide-react'
import { useDeleteMedia, useMediaThumbnail, useRenameMedia } from '@/hooks/useMedia'
import { useCreateOverlay } from '@/hooks/useOverlays'
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
        className="w-full h-full object-cover transition-transform duration-500 group-hover:scale-105" // Ligero zoom al hacer hover
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

function handlePlay(mediaId: number, createOverlay: ReturnType<typeof useCreateOverlay>) {
  createOverlay.mutate({
    media_id: mediaId,
    fullscreen: false,
    position: { x: 100, y: 100 },
    size: { width: 800, height: 600 },
  })
}

export function MediaCard({ media }: MediaCardProps) {
  if (!media) return null
  const [confirmOpen, setConfirmOpen] = useState(false)
  
  // Estados para el renombrado inline
  const [isEditing, setIsEditing] = useState(false)
  const [editedTitle, setEditedTitle] = useState(media.title || media.filename || '')
  const inputRef = useRef<HTMLInputElement>(null)

  const deleteMutation = useDeleteMedia()
  const renameMutation = useRenameMedia()
  const createOverlay = useCreateOverlay()

  // Efecto para enfocar y seleccionar el texto automáticamente al editar
  useEffect(() => {
    if (isEditing && inputRef.current) {
      inputRef.current.focus()
      inputRef.current.select() // Selecciona todo el texto para reescribirlo de inmediato
    }
  }, [isEditing])

  const updateMediaName = (newName: string) => {
    renameMutation.mutate({ id: media.id, title: newName })
  }

  const handleRenameSubmit = () => {
    setIsEditing(false)
    const newTitle = editedTitle.trim()
    const oldTitle = media.title || media.filename || ''
    
    // Solo disparamos la actualización si el nombre realmente cambió y no está vacío
    if (newTitle && newTitle !== oldTitle) {
      updateMediaName(newTitle)
    } else {
      setEditedTitle(oldTitle) // Restaurar si lo dejó en blanco
    }
  }

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter') handleRenameSubmit()
    if (e.key === 'Escape') {
      // Cancelar edición
      setIsEditing(false)
      setEditedTitle(media.title || media.filename || '')
    }
  }

  return (
    <>
      <div className="glass rounded-xl overflow-hidden card-hover group relative flex flex-col">
        <div className="aspect-video bg-zinc-900 flex items-center justify-center relative overflow-hidden">
          <MediaThumbnail media={media} />

          {/* Overlay oscuro para contrastar los botones */}
          <div className="absolute inset-0 bg-black/50 opacity-0 group-hover:opacity-100 transition-opacity duration-300 flex items-center justify-center">
            
            {/* Botón de eliminar (Secundario, esquina superior derecha) */}
            <button
              onClick={(e) => {
                e.stopPropagation() 
                setConfirmOpen(true)
              }}
              className="absolute top-2 right-2 p-2 rounded-lg bg-zinc-900/80 hover:bg-red-600 text-zinc-300 hover:text-white backdrop-blur-sm transition-all shadow-md z-10"
              title="Delete media"
            >
              <Trash2 className="w-4 h-4" />
            </button>

            {/* Botón de Play (Principal, centro) */}
            <button
              onClick={() => handlePlay(media.id, createOverlay)}
              className="w-14 h-14 flex items-center justify-center rounded-full bg-white/20 hover:bg-white/30 text-white backdrop-blur-md transition-all shadow-xl hover:scale-110 border border-white/20 z-10"
              title="Play media"
            >
              <TvMinimalPlay className="w-6 h-6" /> 
            </button>
          </div>
        </div>

        <div className="px-3 py-3 flex flex-col gap-2">
          {/* Lógica de edición Inline */}
          {isEditing ? (
            <input
              ref={inputRef}
              value={editedTitle}
              onChange={(e) => setEditedTitle(e.target.value)}
              onBlur={handleRenameSubmit} // Guarda al hacer clic en cualquier otro lado
              onKeyDown={handleKeyDown}   // Guarda con Enter, cancela con Escape
              className="w-full text-sm font-medium bg-zinc-800 text-zinc-100 px-2 py-1 rounded border border-zinc-600 focus:outline-none focus:border-blue-500 transition-colors"
            />
          ) : (
            <div
              className="group/title flex items-center gap-2 cursor-pointer w-full rounded border border-transparent hover:border-zinc-700/50 hover:bg-zinc-800/30 px-1 -mx-1 transition-all"
              onClick={() => setIsEditing(true)}
              title="Click para renombrar"
            >
              <p className="text-sm font-medium text-zinc-100 truncate select-none">
                {media.title || media.filename}
              </p>
              <Pencil className="w-3 h-3 text-zinc-500 opacity-0 group-hover/title:opacity-100 transition-opacity shrink-0" />
            </div>
          )}

          <div className="flex items-center justify-between gap-2 mt-auto">
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