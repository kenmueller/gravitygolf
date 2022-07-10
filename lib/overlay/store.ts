import { writable } from 'svelte/store'

import type Overlay from '.'

const overlay = writable<Overlay | null>(null)

export default overlay
