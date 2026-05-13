import { useState } from 'react'
import { FolderPlus, FolderOpen, Trash2, ChevronRight } from 'lucide-react'
import { useCategoryList, useDeleteCategory } from '@/hooks/useCategories'
import { CreateCategoryModal } from './components/CreateCategoryModal'
import { CategoryDetailModal } from './components/CategoryDetailModal'
import { Button } from '@/shared/components/Button'
import { ConfirmDialog } from '@/shared/components/ConfirmDialog'
import { EmptyState } from '@/shared/components/EmptyState'
import { SkeletonRow } from '@/shared/components/SkeletonCard'
import type { CategorySummary } from '@/types'

export function CategoriesPage() {
  const { data: categories = [], isLoading } = useCategoryList()
  const deleteCategory = useDeleteCategory()
  const [createOpen, setCreateOpen] = useState(false)
  const [selectedCategory, setSelectedCategory] = useState<CategorySummary | null>(null)
  const [deleteTarget, setDeleteTarget] = useState<CategorySummary | null>(null)

  return (
    <div className="space-y-5">
      <div className="flex items-center gap-4">
        <div>
          <h1 className="text-2xl font-bold text-zinc-100">Categories</h1>
          <p className="text-sm text-zinc-500 mt-0.5">{categories.length} categories</p>
        </div>
        <div className="ml-auto">
          <Button
            variant="primary"
            icon={<FolderPlus className="w-4 h-4" />}
            onClick={() => setCreateOpen(true)}
          >
            New Category
          </Button>
        </div>
      </div>

      {isLoading ? (
        <div className="space-y-2">
          {Array.from({ length: 4 }).map((_, i) => <SkeletonRow key={i} />)}
        </div>
      ) : categories.length === 0 ? (
        <EmptyState
          icon={<FolderOpen className="w-12 h-12" />}
          title="No categories yet"
          description="Organize your media into categories for easier management."
          action={
            <Button
              variant="primary"
              icon={<FolderPlus className="w-4 h-4" />}
              onClick={() => setCreateOpen(true)}
            >
              Create Category
            </Button>
          }
        />
      ) : (
        <div className="grid sm:grid-cols-2 lg:grid-cols-3 gap-3">
          {categories.map((cat) => (
            <CategoryCard
              key={cat.id}
              category={cat}
              onOpen={() => setSelectedCategory(cat)}
              onDelete={() => setDeleteTarget(cat)}
            />
          ))}
        </div>
      )}

      <CreateCategoryModal open={createOpen} onClose={() => setCreateOpen(false)} />

      <CategoryDetailModal
        category={selectedCategory}
        onClose={() => setSelectedCategory(null)}
      />

      <ConfirmDialog
        open={Boolean(deleteTarget)}
        onClose={() => setDeleteTarget(null)}
        onConfirm={() =>
          deleteTarget &&
          deleteCategory.mutate(deleteTarget.id, { onSuccess: () => setDeleteTarget(null) })
        }
        title="Delete Category"
        message={`Delete "${deleteTarget?.name}"? This will not delete the media files themselves.`}
        loading={deleteCategory.isPending}
      />
    </div>
  )
}

interface CategoryCardProps {
  category: CategorySummary
  onOpen: () => void
  onDelete: () => void
}

function CategoryCard({ category, onOpen, onDelete }: CategoryCardProps) {
  return (
    <div className="glass rounded-xl p-4 card-hover group flex flex-col gap-3">
      <div className="flex items-start justify-between gap-2">
        <div className="flex items-center gap-3 min-w-0">
          <div className="w-9 h-9 rounded-lg bg-violet-600/20 flex items-center justify-center flex-shrink-0">
            <FolderOpen className="w-4 h-4 text-violet-400" />
          </div>
          <div className="min-w-0">
            <p className="font-semibold text-zinc-100 truncate">{category.name}</p>
            <p className="text-xs text-zinc-500 mt-0.5">ID: {category.id}</p>
          </div>
        </div>
        <button
          onClick={(e) => { e.stopPropagation(); onDelete() }}
          className="p-1.5 text-zinc-600 hover:text-red-400 hover:bg-red-600/10 rounded-lg transition-colors opacity-0 group-hover:opacity-100 flex-shrink-0"
        >
          <Trash2 className="w-3.5 h-3.5" />
        </button>
      </div>

      <button
        onClick={onOpen}
        className="flex items-center gap-1.5 text-xs font-medium text-violet-400 hover:text-violet-300 transition-colors mt-auto"
      >
        Manage media
        <ChevronRight className="w-3.5 h-3.5" />
      </button>
    </div>
  )
}
