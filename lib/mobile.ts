import type { Writable } from 'svelte/store'
import { derived } from 'svelte/store'

import { session } from '$app/stores'

import type Session from './session'

const mobile = derived(session as Writable<Session>, ({ mobile }) => mobile)

export default mobile
