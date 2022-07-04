import { derived } from 'svelte/store'

import stars from '.'

const totalCommunityStars = derived(
	stars,
	$stars =>
		$stars && Object.values($stars).reduce((total, level) => total + level, 0)
)

export default totalCommunityStars
