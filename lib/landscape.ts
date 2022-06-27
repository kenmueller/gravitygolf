import { readable } from 'svelte/store'

import { browser } from '$app/env'

const landscape = readable<boolean | null>(null, set => {
	if (!browser) return set(null)

	const media = window.matchMedia('(orientation: landscape)')
	set(media.matches)

	const onMediaChange = ({ matches }: MediaQueryListEvent) => {
		set(matches)
	}

	media.addEventListener('change', onMediaChange)

	return () => {
		media.removeEventListener('change', onMediaChange)
	}
})

export default landscape
