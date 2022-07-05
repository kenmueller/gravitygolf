import type { Writable } from 'svelte/store'
import { get } from 'svelte/store'
import { derived } from 'svelte/store'

import { browser } from '$app/env'

import type Record from '../record'
import query from '../query'
import isInitial from '../initial'
import load from './load'

const communityLevels = derived<Writable<string>, Record[] | null>(
	query,
	($query, set) => {
		set(null)

		if (!browser) return
		if (get(isInitial)) return isInitial.set(false)

		const cancel = { current: false }

		void load($query, set, cancel)

		return () => {
			cancel.current = true
		}
	}
)

export default communityLevels
