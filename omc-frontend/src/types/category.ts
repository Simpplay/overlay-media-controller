export interface CategorySummary {
  id: number
  name: string
}

export interface CategoryMediaItem {
  id: number
  title: string
  filename: string
  contentType: string
}

export interface CategoryDetail {
  id: number
  name: string
  media: CategoryMediaItem[]
}

export interface CreateCategoryPayload {
  name: string
}
