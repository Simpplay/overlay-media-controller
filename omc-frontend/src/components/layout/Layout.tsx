import { Outlet } from 'react-router-dom'
import { Sidebar } from './Sidebar'

export function Layout() {
  return (
    <div className="min-h-screen bg-zinc-950">
      <Sidebar />
      <main className="lg:pl-60 pt-14 lg:pt-0 min-h-screen">
        <div className="max-w-screen-2xl mx-auto px-4 sm:px-6 py-6">
          <Outlet />
        </div>
      </main>
    </div>
  )
}
