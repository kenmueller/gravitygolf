import type { Writable } from 'svelte/store'
import { derived } from 'svelte/store'

import type CommunityLevelRecord from '../record'
import query from '../query'
import initial from './initial'

const communityLevels = derived<
	[Writable<string>, Writable<CommunityLevelRecord[]>],
	CommunityLevelRecord[]
>([query, initial], ([$query, $initial], set) => {
	set($initial)
})

export default communityLevels
