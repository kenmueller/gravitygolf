import { writable } from 'svelte/store'

import { browser } from '$app/env'

import getStars from './get'

const stars = writable<number[] | null>(null, set => {
	set(browser ? getStars() : null)
})

export default stars
