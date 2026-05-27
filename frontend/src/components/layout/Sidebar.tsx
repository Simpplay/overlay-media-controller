import { NavLink } from 'react-router-dom'
import { LayoutGrid, FolderOpen, Layers, Home, Menu, X, Gamepad2 } from 'lucide-react'
import { useState } from 'react'

const navItems = [
  { to: '/', icon: Home, label: 'Dashboard', end: true },
  { to: '/media', icon: LayoutGrid, label: 'Media' },
  { to: '/categories', icon: FolderOpen, label: 'Categories' },
  { to: '/overlays', icon: Layers, label: 'Overlays' },
  { to: '/streamdeck', icon: Gamepad2, label: 'StreamDeck' },
]

export function Sidebar() {
  const [mobileOpen, setMobileOpen] = useState(false)

  return (
    <>
      {/* Mobile header */}
      <header className="lg:hidden fixed top-0 left-0 right-0 z-40 flex items-center gap-3 px-4 py-3 glass border-b border-zinc-800">
        <button
          onClick={() => setMobileOpen(!mobileOpen)}
          className="p-1.5 rounded-lg hover:bg-zinc-800 transition-colors text-zinc-400 hover:text-zinc-100"
        >
          {mobileOpen ? <X className="w-5 h-5" /> : <Menu className="w-5 h-5" />}
        </button>
        <span className="font-bold text-zinc-100 tracking-tight">OMC</span>
      </header>

      {/* Mobile overlay */}
      {mobileOpen && (
        <div
          className="lg:hidden fixed inset-0 z-30 bg-black/60"
          onClick={() => setMobileOpen(false)}
        />
      )}

      {/* Sidebar */}
      <aside
        className={[
          'fixed top-0 left-0 h-full z-40 w-60 flex flex-col glass border-r border-zinc-800/50',
          'transition-transform duration-300',
          'lg:translate-x-0',
          mobileOpen ? 'translate-x-0' : '-translate-x-full lg:translate-x-0',
        ].join(' ')}
      >
        {/* Logo */}
        <div className="flex items-center gap-2.5 px-5 py-5 border-b border-zinc-800/50">
          <div className="w-8 h-8 rounded-lg flex items-center justify-center">
            <img src="/favicon.svg" alt="Company Logo"></img>
          </div>
          <div>
            <p className="text-sm font-bold text-zinc-100 leading-tight">OMC</p>
            <p className="text-xs text-zinc-500 leading-tight">Media Controller</p>
          </div>
        </div>

        {/* Nav */}
        <nav className="flex-1 px-3 py-4 space-y-1 overflow-y-auto">
          {navItems.map(({ to, icon: Icon, label, end }) => (
            <NavLink
              key={to}
              to={to}
              end={end}
              onClick={() => setMobileOpen(false)}
              className={({ isActive }) =>
                [
                  'flex items-center gap-3 px-3 py-2.5 rounded-lg text-sm font-medium transition-all duration-150',
                  isActive
                    ? 'bg-violet-600/20 text-violet-300 border border-violet-600/30'
                    : 'text-zinc-400 hover:text-zinc-100 hover:bg-zinc-800',
                ].join(' ')
              }
            >
              <Icon className="w-4 h-4 flex-shrink-0" />
              {label}
            </NavLink>
          ))}
        </nav>

        {/* Footer */}
        <div className="px-5 py-4 border-t border-zinc-800/50">
          <p className="text-xs text-zinc-600">API: localhost:8080</p>
        </div>
      </aside>
    </>
  )
}
