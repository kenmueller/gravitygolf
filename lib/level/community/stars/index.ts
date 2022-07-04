import { writable } from 'svelte/store'

import { browser } from '$app/env'

import getStars from './get'

const communityStars = writable<Record<string, number> | null>(null, set => {
	set(browser ? getStars() : null)
})

export default communityStars
