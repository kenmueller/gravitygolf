import { set } from '$lib/storage'
import getCommunityStars from './get'
import store from '.'

const setCommunityStars = (level: string, stars: number) => {
	try {
		const map = getCommunityStars()

		if (!level) throw new Error('Invalid level')

		map[level] = Math.max(map[level] ?? 0, stars)
		set('communityStars', JSON.stringify(map))

		store.set(map)
	} catch (error) {
		console.error(error)
	}
}

export default setCommunityStars
