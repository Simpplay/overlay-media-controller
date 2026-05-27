import { useSoundboardDevices, useSetSoundboardDevice } from '@/hooks/useSoundboard'
import { Headphones } from 'lucide-react'

export function SoundboardDeviceSelect() {
  const { data: devices = [], isLoading } = useSoundboardDevices()
  const setDevice = useSetSoundboardDevice()

  if (isLoading) {
    return <div className="h-8 w-48 bg-zinc-800/50 rounded-lg animate-pulse" />
  }

  if (devices.length === 0) return null

  const selectedDevice = devices.find(d => d.selected)?.device_id || 0

  return (
    <div className="flex items-center gap-2 bg-zinc-800/30 hover:bg-zinc-800/50 transition-colors px-3 py-1.5 rounded-lg border border-zinc-700/50">
      <Headphones className="w-4 h-4 text-zinc-400" />
      <select
        className="bg-transparent text-sm text-zinc-300 outline-none cursor-pointer w-full appearance-none pr-4"
        value={selectedDevice}
        onChange={(e) => setDevice.mutate({ device_id: Number(e.target.value) })}
        disabled={setDevice.isPending}
      >
        <option value="" disabled className="bg-zinc-900 text-zinc-500">
          Select output device
        </option>
        {devices.map((device) => (
          <option 
            key={device.device_id} 
            value={device.device_id} 
            className="bg-zinc-900 text-zinc-200"
          >
            {device.name}
          </option>
        ))}
      </select>
    </div>
  )
}