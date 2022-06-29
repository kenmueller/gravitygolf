import { derived } from 'svelte/store'

import stars from '.'

const totalStars = derived(
	stars,
	$stars => $stars && $stars.reduce((total, level) => total + level, 0)
)

export default totalStars
