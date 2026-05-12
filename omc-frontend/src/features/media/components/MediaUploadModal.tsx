import { useState, useRef, useCallback } from 'react'
import { Upload, X, FileVideo, FileAudio, FileImage, File } from 'lucide-react'
import { Modal } from '@/shared/components/Modal'
import { Button } from '@/shared/components/Button'
import { useUploadMedia } from '@/hooks/useMedia'
import type { UploadMediaParams } from '@/types'

interface MediaUploadModalProps {
  open: boolean
  onClose: () => void
}

function getFileIcon(type: string) {
  if (type.startsWith('video/')) return FileVideo
  if (type.startsWith('audio/')) return FileAudio
  if (type.startsWith('image/')) return FileImage
  return File
}

interface PendingFile {
  file: File
  title: string
}

export function MediaUploadModal({ open, onClose }: MediaUploadModalProps) {
  const [dragOver, setDragOver] = useState(false)
  const [pending, setPending] = useState<PendingFile[]>([])
  const inputRef = useRef<HTMLInputElement>(null)
  const uploadMutation = useUploadMedia()

  const addFiles = (incoming: FileList | null) => {
    if (!incoming) return
    const next: PendingFile[] = Array.from(incoming).map((file) => ({
      file,
      title: '',
    }))
    setPending((prev) => [...prev, ...next])
  }

  const onDrop = useCallback((e: React.DragEvent) => {
    e.preventDefault()
    setDragOver(false)
    addFiles(e.dataTransfer.files)
  }, [])

  const updateTitle = (index: number, title: string) => {
    setPending((prev) => prev.map((p, i) => (i === index ? { ...p, title } : p)))
  }

  const handleUpload = async () => {
    for (const { file, title } of pending) {
      const params: UploadMediaParams = { file }
      if (title.trim()) params.title = title.trim()
      await uploadMutation.mutateAsync(params)
    }
    setPending([])
    onClose()
  }

  const removeFile = (index: number) => {
    setPending((prev) => prev.filter((_, i) => i !== index))
  }

  const handleClose = () => {
    setPending([])
    onClose()
  }

  return (
    <Modal open={open} onClose={handleClose} title="Upload Media" size="md">
      <div className="space-y-4">
        <div
          onDragOver={(e) => { e.preventDefault(); setDragOver(true) }}
          onDragLeave={() => setDragOver(false)}
          onDrop={onDrop}
          onClick={() => inputRef.current?.click()}
          className={[
            'border-2 border-dashed rounded-xl p-8 flex flex-col items-center gap-3 cursor-pointer transition-all duration-150',
            dragOver
              ? 'border-violet-500 bg-violet-600/10'
              : 'border-zinc-700 hover:border-zinc-600 hover:bg-zinc-800/50',
          ].join(' ')}
        >
          <Upload className="w-8 h-8 text-zinc-500" />
          <div className="text-center">
            <p className="text-sm font-medium text-zinc-300">Drop files here or click to browse</p>
            <p className="text-xs text-zinc-600 mt-1">Video, audio, and image files supported</p>
          </div>
          <input
            ref={inputRef}
            type="file"
            multiple
            className="hidden"
            onChange={(e) => addFiles(e.target.files)}
          />
        </div>

        {pending.length > 0 && (
          <ul className="space-y-2 max-h-56 overflow-y-auto">
            {pending.map(({ file, title }, i) => {
              const Icon = getFileIcon(file.type)
              return (
                <li key={i} className="flex items-start gap-3 px-3 py-2.5 bg-zinc-900 rounded-lg">
                  <Icon className="w-4 h-4 text-zinc-500 flex-shrink-0 mt-2" />
                  <div className="flex-1 min-w-0 space-y-1.5">
                    <p className="text-sm text-zinc-400 truncate">{file.name}</p>
                    <input
                      type="text"
                      placeholder="Title (optional)"
                      value={title}
                      onChange={(e) => updateTitle(i, e.target.value)}
                      className="w-full text-xs px-2 py-1 bg-zinc-800 border border-zinc-700 rounded text-zinc-200 placeholder:text-zinc-600 focus:outline-none focus:border-violet-500"
                    />
                  </div>
                  <button
                    onClick={() => removeFile(i)}
                    className="text-zinc-500 hover:text-red-400 transition-colors mt-1.5 flex-shrink-0"
                  >
                    <X className="w-4 h-4" />
                  </button>
                </li>
              )
            })}
          </ul>
        )}

        <div className="flex gap-2 justify-end pt-2">
          <Button variant="ghost" onClick={handleClose}>
            Cancel
          </Button>
          <Button
            variant="primary"
            onClick={handleUpload}
            disabled={pending.length === 0}
            loading={uploadMutation.isPending}
            icon={<Upload className="w-4 h-4" />}
          >
            Upload {pending.length > 0 ? `(${pending.length})` : ''}
          </Button>
        </div>
      </div>
    </Modal>
  )
}
