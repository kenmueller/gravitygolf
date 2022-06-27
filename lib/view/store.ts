import type { Writable } from 'svelte/store'
import { derived } from 'svelte/store'

import { browser } from '$app/env'
import { session } from '$app/stores'

import type Session from '$lib/session'
import type View from '.'
import getView from './get'

const view = derived<Writable<Session>, View | null>(
	session as Writable<Session>,
	({ mobile }, set) => {
		if (!browser) return set(null)

		const setView = () => {
			set(getView(mobile))
		}

		setView()
		window.addEventListener('resize', setView)

		return () => {
			window.removeEventListener('resize', setView)
		}
	}
)

export default view
