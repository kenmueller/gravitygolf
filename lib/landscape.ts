import { readable } from 'svelte/store'

import { browser } from '$app/env'

const landscape = readable<boolean | null>(null, set => {
	if (!(browser && typeof window.orientation === 'number')) return set(null)

	const setLandscape = () => {
		set(Math.abs(window.orientation) === 90)
	}

	setLandscape()
	window.addEventListener('orientationchange', setLandscape)

	return () => {
		window.removeEventListener('orientationchange', setLandscape)
	}
})

export default landscape
