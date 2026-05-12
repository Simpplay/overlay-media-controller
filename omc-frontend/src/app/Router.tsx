import { BrowserRouter, Routes, Route } from 'react-router-dom'
import { Layout } from '@/components/layout/Layout'
import { Dashboard } from '@/pages/Dashboard'
import { MediaPage } from '@/features/media/MediaPage'
import { CategoriesPage } from '@/features/categories/CategoriesPage'
import { OverlaysPage } from '@/features/overlays/OverlaysPage'
import { StreamDeckPage } from '@/features/streamdeck/StreamDeckPage'

export function Router() {
  return (
    <BrowserRouter>
      <Routes>
        <Route element={<Layout />}>
          <Route index element={<Dashboard />} />
          <Route path="/media" element={<MediaPage />} />
          <Route path="/categories" element={<CategoriesPage />} />
          <Route path="/overlays" element={<OverlaysPage />} />
          <Route path="/streamdeck" element={<StreamDeckPage />} />
        </Route>
      </Routes>
    </BrowserRouter>
  )
}
