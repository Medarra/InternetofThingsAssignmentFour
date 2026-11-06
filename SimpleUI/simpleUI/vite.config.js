import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react' // FIXED: Swapped to the standard react plugin path
import { viteSingleFile } from 'vite-plugin-singlefile'

export default defineConfig({
  plugins: [react(), viteSingleFile()],
  base: './'
})
