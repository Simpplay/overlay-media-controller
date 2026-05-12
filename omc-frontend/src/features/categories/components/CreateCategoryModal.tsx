import { useState } from 'react'
import { FolderPlus } from 'lucide-react'
import { Modal } from '@/shared/components/Modal'
import { Button } from '@/shared/components/Button'
import { Input } from '@/shared/components/Input'
import { useCreateCategory } from '@/hooks/useCategories'

interface CreateCategoryModalProps {
  open: boolean
  onClose: () => void
}

export function CreateCategoryModal({ open, onClose }: CreateCategoryModalProps) {
  const [name, setName] = useState('')
  const [error, setError] = useState('')
  const createMutation = useCreateCategory()

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault()
    if (!name.trim()) {
      setError('Category name is required')
      return
    }
    createMutation.mutate({ name: name.trim() }, {
      onSuccess: () => {
        setName('')
        setError('')
        onClose()
      },
    })
  }

  const handleClose = () => {
    setName('')
    setError('')
    onClose()
  }

  return (
    <Modal open={open} onClose={handleClose} title="Create Category" size="sm">
      <form onSubmit={handleSubmit} className="space-y-4">
        <Input
          label="Category name"
          placeholder="e.g. Intro Videos"
          value={name}
          onChange={(e) => { setName(e.target.value); setError('') }}
          error={error}
          autoFocus
        />
        <div className="flex gap-2 justify-end">
          <Button variant="ghost" type="button" onClick={handleClose}>
            Cancel
          </Button>
          <Button
            variant="primary"
            type="submit"
            loading={createMutation.isPending}
            icon={<FolderPlus className="w-4 h-4" />}
          >
            Create
          </Button>
        </div>
      </form>
    </Modal>
  )
}
