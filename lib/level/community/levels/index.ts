import { get, type Writable } from 'svelte/store'
import { derived } from 'svelte/store'

import { browser } from '$app/env'

import type CommunityLevelRecord from '../record'
import query from '../query'
import initial from './initial'
import getCommunityLevels from './get'
import isInitial from '../initial'

const communityLevels = derived<Writable<string>, CommunityLevelRecord[]>(
	query,
	($query, set) => {
		set(get(initial))

		if (!browser) return
		if (get(isInitial)) return isInitial.set(false)

		let cancel = false

		getCommunityLevels(fetch, $query)
			.then(levels => {
				if (cancel) return

				console.log(levels)

				initial.set(levels)
				set(levels)
			})
			.catch(console.error)

		return () => {
			cancel = true
		}
	}
)

export default communityLevels
